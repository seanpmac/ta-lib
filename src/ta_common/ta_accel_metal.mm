#if !defined(TA_ACCEL_HAS_METAL)
#error "ta_accel_metal.mm compiled without TA_ACCEL_HAS_METAL defined"
#endif

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <vector>
#include <string.h>

#include "ta_accel.h"

namespace {
static id<MTLDevice> gDevice = nil;
static id<MTLCommandQueue> gQueue = nil;
static id<MTLComputePipelineState> gScanState = nil;
static id<MTLComputePipelineState> gAddOffsetsState = nil;
static id<MTLComputePipelineState> gSMAPrefixState = nil;
static id<MTLComputePipelineState> gWMAState = nil;
static id<MTLComputePipelineState> gScanStateDouble = nil;
static id<MTLComputePipelineState> gAddOffsetsStateDouble = nil;
static id<MTLComputePipelineState> gSMAPrefixStateDouble = nil;
static id<MTLComputePipelineState> gWMAStateDouble = nil;
static bool gMetalReady = false;
static bool gSupportsDoublePrecision = false;

static id<MTLBuffer> gInputBuffer = nil;
static id<MTLBuffer> gPrefixBuffer = nil;
static id<MTLBuffer> gPrefixWeightedBuffer = nil;
static id<MTLBuffer> gOutputBuffer = nil;
static id<MTLBuffer> gBlockBuffer = nil;
static NSUInteger gInputCapacity = 0;
static NSUInteger gPrefixCapacity = 0;
static NSUInteger gPrefixWeightedCapacity = 0;
static NSUInteger gOutputCapacity = 0;
static NSUInteger gBlockCapacity = 0;

static id<MTLBuffer> gInputBufferDouble = nil;
static id<MTLBuffer> gPrefixBufferDouble = nil;
static id<MTLBuffer> gPrefixWeightedBufferDouble = nil;
static id<MTLBuffer> gOutputBufferDouble = nil;
static id<MTLBuffer> gBlockBufferDouble = nil;
static NSUInteger gInputCapacityDouble = 0;
static NSUInteger gPrefixCapacityDouble = 0;
static NSUInteger gPrefixWeightedCapacityDouble = 0;
static NSUInteger gOutputCapacityDouble = 0;
static NSUInteger gBlockCapacityDouble = 0;

static std::vector<float> gScratchDoubleInput;
static std::vector<float> gScratchOutput;
static std::vector<double> gScratchWeightedDouble;

static const NSUInteger kThreadgroupSize = 256;

static const char *kSMAKernelSource =
"#include <metal_stdlib>\n"
"using namespace metal;\n"
"constant uint kThreadgroupSizeConst = 256;\n"
"kernel void prefix_scan(const device float *inValues [[buffer(0)]],\n"
"                        device float *prefix [[buffer(1)]],\n"
"                        device float *blockSums [[buffer(2)]],\n"
"                        constant uint &count [[buffer(3)]],\n"
"                        uint gid [[thread_position_in_grid]],\n"
"                        uint tid [[thread_index_in_threadgroup]],\n"
"                        uint groupId [[threadgroup_position_in_grid]])\n"
"{\n"
"    threadgroup float shared[kThreadgroupSizeConst];\n"
"    float value = 0.0f;\n"
"    if (gid < count)\n"
"        value = inValues[gid];\n"
"    shared[tid] = value;\n"
"    threadgroup_barrier(mem_flags::mem_threadgroup);\n"
"    for (uint offset = 1; offset < kThreadgroupSizeConst; offset <<= 1) {\n"
"        float tmp = 0.0f;\n"
"        if (tid >= offset)\n"
"            tmp = shared[tid - offset];\n"
"        threadgroup_barrier(mem_flags::mem_threadgroup);\n"
"        shared[tid] += tmp;\n"
"        threadgroup_barrier(mem_flags::mem_threadgroup);\n"
"    }\n"
"    if (gid < count)\n"
"        prefix[gid] = shared[tid];\n"
"    uint blockStart = groupId * kThreadgroupSizeConst;\n"
"    if (blockStart < count) {\n"
"        uint blockEnd = min(blockStart + kThreadgroupSizeConst, count) - 1;\n"
"        if (gid == blockEnd)\n"
"            blockSums[groupId] = shared[tid];\n"
"    }\n"
"}\n"
"kernel void add_block_offsets(device float *prefix [[buffer(0)]],\n"
"                              const device float *blockOffsets [[buffer(1)]],\n"
"                              constant uint &count [[buffer(2)]],\n"
"                              uint gid [[thread_position_in_grid]])\n"
"{\n"
"    if (gid >= count)\n"
"        return;\n"
"    uint blockId = gid / kThreadgroupSizeConst;\n"
"    prefix[gid] += blockOffsets[blockId];\n"
"}\n"
"kernel void sma_from_prefix(const device float *prefix [[buffer(0)]],\n"
"                            device float *outValues [[buffer(1)]],\n"
"                            constant uint &period [[buffer(2)]],\n"
"                            constant uint &outputCount [[buffer(3)]],\n"
"                            uint gid [[thread_position_in_grid]])\n"
"{\n"
"    if (gid >= outputCount)\n"
"        return;\n"
"    uint endIndex = gid + period - 1;\n"
"    float endValue = prefix[endIndex];\n"
"    float startValue = (gid == 0) ? 0.0f : prefix[gid - 1];\n"
"    outValues[gid] = (endValue - startValue) / float(period);\n"
"}\n"
"kernel void wma_from_samples(const device float *input [[buffer(0)]],\n"
"                            device float *outValues [[buffer(1)]],\n"
"                            constant uint &period [[buffer(2)]],\n"
"                            constant uint &outputCount [[buffer(3)]],\n"
"                            uint gid [[thread_position_in_grid]])\n"
"{\n"
"    if (gid >= outputCount)\n"
"        return;\n"
"    uint start = gid;\n"
"    float weighted = 0.0f;\n"
"    for (uint k = 0; k < period; ++k) {\n"
"        float value = input[start + k];\n"
"        weighted += float(k + 1u) * value;\n"
"    }\n"
"    float denom = float(period) * float(period + 1u) * 0.5f;\n"
"    outValues[gid] = weighted / denom;\n"
"}\n";

static const char *kSMAKernelSourceDouble =
"#include <metal_stdlib>\n"
"using namespace metal;\n"
"constant uint kThreadgroupSizeConst = 256;\n"
"kernel void prefix_scan_double(const device double *inValues [[buffer(0)]],\n"
"                               device double *prefix [[buffer(1)]],\n"
"                               device double *blockSums [[buffer(2)]],\n"
"                               constant uint &count [[buffer(3)]],\n"
"                               uint gid [[thread_position_in_grid]],\n"
"                               uint tid [[thread_index_in_threadgroup]],\n"
"                               uint groupId [[threadgroup_position_in_grid]])\n"
"{\n"
"    threadgroup double shared[kThreadgroupSizeConst];\n"
"    double value = 0.0;\n"
"    if (gid < count)\n"
"        value = inValues[gid];\n"
"    shared[tid] = value;\n"
"    threadgroup_barrier(mem_flags::mem_threadgroup);\n"
"    for (uint offset = 1; offset < kThreadgroupSizeConst; offset <<= 1) {\n"
"        double tmp = 0.0;\n"
"        if (tid >= offset)\n"
"            tmp = shared[tid - offset];\n"
"        threadgroup_barrier(mem_flags::mem_threadgroup);\n"
"        shared[tid] += tmp;\n"
"        threadgroup_barrier(mem_flags::mem_threadgroup);\n"
"    }\n"
"    if (gid < count)\n"
"        prefix[gid] = shared[tid];\n"
"    uint blockStart = groupId * kThreadgroupSizeConst;\n"
"    if (blockStart < count) {\n"
"        uint blockEnd = min(blockStart + kThreadgroupSizeConst, count) - 1;\n"
"        if (gid == blockEnd)\n"
"            blockSums[groupId] = shared[tid];\n"
"    }\n"
"}\n"
"kernel void add_block_offsets_double(device double *prefix [[buffer(0)]],\n"
"                                     const device double *blockOffsets [[buffer(1)]],\n"
"                                     constant uint &count [[buffer(2)]],\n"
"                                     uint gid [[thread_position_in_grid]])\n"
"{\n"
"    if (gid >= count)\n"
"        return;\n"
"    uint blockId = gid / kThreadgroupSizeConst;\n"
"    prefix[gid] += blockOffsets[blockId];\n"
"}\n"
"kernel void sma_from_prefix_double(const device double *prefix [[buffer(0)]],\n"
"                                   device double *outValues [[buffer(1)]],\n"
"                                   constant uint &period [[buffer(2)]],\n"
"                                   constant uint &outputCount [[buffer(3)]],\n"
"                                   uint gid [[thread_position_in_grid]])\n"
"{\n"
"    if (gid >= outputCount)\n"
"        return;\n"
"    uint endIndex = gid + period - 1;\n"
"    double endValue = prefix[endIndex];\n"
"    double startValue = (gid == 0) ? 0.0 : prefix[gid - 1];\n"
"    outValues[gid] = (endValue - startValue) / double(period);\n"
"}\n"
"kernel void wma_from_prefix_double(const device double *prefix [[buffer(0)]],\n"
"                                   const device double *weightedPrefix [[buffer(1)]],\n"
"                                   device double *outValues [[buffer(2)]],\n"
"                                   constant uint &period [[buffer(3)]],\n"
"                                   constant uint &outputCount [[buffer(4)]],\n"
"                                   uint gid [[thread_position_in_grid]])\n"
"{\n"
"    if (gid >= outputCount)\n"
"        return;\n"
"    uint endIndex = gid + period - 1;\n"
"    double endValue = prefix[endIndex];\n"
"    double startValue = (gid == 0) ? 0.0 : prefix[gid - 1];\n"
"    double weightedEnd = weightedPrefix[endIndex];\n"
"    double weightedStart = (gid == 0) ? 0.0 : weightedPrefix[gid - 1];\n"
"    double baseSum = endValue - startValue;\n"
"    double numerator = (weightedEnd - weightedStart) - double(gid) * baseSum;\n"
"    double denom = double(period) * double(period + 1u) * 0.5;\n"
"    outValues[gid] = numerator / denom;\n"
"}\n";

static void cleanup_resources(void)
{
    if (gSMAPrefixState) {
        [gSMAPrefixState release];
        gSMAPrefixState = nil;
    }
    if (gWMAState) {
        [gWMAState release];
        gWMAState = nil;
    }
    if (gSMAPrefixStateDouble) {
        [gSMAPrefixStateDouble release];
        gSMAPrefixStateDouble = nil;
    }
    if (gWMAStateDouble) {
        [gWMAStateDouble release];
        gWMAStateDouble = nil;
    }
    if (gAddOffsetsState) {
        [gAddOffsetsState release];
        gAddOffsetsState = nil;
    }
    if (gAddOffsetsStateDouble) {
        [gAddOffsetsStateDouble release];
        gAddOffsetsStateDouble = nil;
    }
    if (gScanState) {
        [gScanState release];
        gScanState = nil;
    }
    if (gScanStateDouble) {
        [gScanStateDouble release];
        gScanStateDouble = nil;
    }
    if (gQueue) {
        [gQueue release];
        gQueue = nil;
    }
    if (gDevice) {
        [gDevice release];
        gDevice = nil;
    }
    if (gInputBuffer) {
        [gInputBuffer release];
        gInputBuffer = nil;
        gInputCapacity = 0;
    }
    if (gPrefixBuffer) {
        [gPrefixBuffer release];
        gPrefixBuffer = nil;
        gPrefixCapacity = 0;
    }
    if (gPrefixWeightedBuffer) {
        [gPrefixWeightedBuffer release];
        gPrefixWeightedBuffer = nil;
        gPrefixWeightedCapacity = 0;
    }
    if (gOutputBuffer) {
        [gOutputBuffer release];
        gOutputBuffer = nil;
        gOutputCapacity = 0;
    }
    if (gBlockBuffer) {
        [gBlockBuffer release];
        gBlockBuffer = nil;
        gBlockCapacity = 0;
    }
    if (gInputBufferDouble) {
        [gInputBufferDouble release];
        gInputBufferDouble = nil;
        gInputCapacityDouble = 0;
    }
    if (gPrefixBufferDouble) {
        [gPrefixBufferDouble release];
        gPrefixBufferDouble = nil;
        gPrefixCapacityDouble = 0;
    }
    if (gPrefixWeightedBufferDouble) {
        [gPrefixWeightedBufferDouble release];
        gPrefixWeightedBufferDouble = nil;
        gPrefixWeightedCapacityDouble = 0;
    }
    if (gOutputBufferDouble) {
        [gOutputBufferDouble release];
        gOutputBufferDouble = nil;
        gOutputCapacityDouble = 0;
    }
    if (gBlockBufferDouble) {
        [gBlockBufferDouble release];
        gBlockBufferDouble = nil;
        gBlockCapacityDouble = 0;
    }
    gMetalReady = false;
}

static bool ensure_pipeline(void)
{
    if (gMetalReady && gDevice && gQueue && gScanState && gAddOffsetsState && gSMAPrefixState && gWMAState && (!gSupportsDoublePrecision || (gScanStateDouble && gAddOffsetsStateDouble && gSMAPrefixStateDouble && gWMAStateDouble)))
        return true;

    cleanup_resources();

    @autoreleasepool {
        gDevice = MTLCreateSystemDefaultDevice();
        if (!gDevice)
            return false;

        gQueue = [gDevice newCommandQueue];
        if (!gQueue) {
            cleanup_resources();
            return false;
        }

        NSError *error = nil;
        NSString *source = [[NSString alloc] initWithUTF8String:kSMAKernelSource];
        if (!source) {
            cleanup_resources();
            return false;
        }

        MTLCompileOptions *options = [[MTLCompileOptions alloc] init];
        id<MTLLibrary> library = [gDevice newLibraryWithSource:source options:options error:&error];
        [options release];
        [source release];

        if (!library) {
            cleanup_resources();
            return false;
        }

        id<MTLFunction> scanFunction = [library newFunctionWithName:@"prefix_scan"];
        id<MTLFunction> addFunction = [library newFunctionWithName:@"add_block_offsets"];
        id<MTLFunction> smaFunction = [library newFunctionWithName:@"sma_from_prefix"];
        id<MTLFunction> wmaFunction = [library newFunctionWithName:@"wma_from_samples"];

        bool haveFloatPipelines = (scanFunction && addFunction && smaFunction && wmaFunction);

        if (!haveFloatPipelines) {
            if (scanFunction) [scanFunction release];
            if (addFunction) [addFunction release];
            if (smaFunction) [smaFunction release];
            if (wmaFunction) [wmaFunction release];
            [library release];
            cleanup_resources();
            return false;
        }

        gScanState = [gDevice newComputePipelineStateWithFunction:scanFunction error:&error];
        gAddOffsetsState = [gDevice newComputePipelineStateWithFunction:addFunction error:&error];
        gSMAPrefixState = [gDevice newComputePipelineStateWithFunction:smaFunction error:&error];
        gWMAState = [gDevice newComputePipelineStateWithFunction:wmaFunction error:&error];
        gSupportsDoublePrecision = false;

        BOOL deviceSupportsFP64 = NO;
        SEL fp64Selector = @selector(supports64BitFloat);
        if ([gDevice respondsToSelector:fp64Selector]) {
            IMP fp64Imp = [(id)gDevice methodForSelector:fp64Selector];
            BOOL (*supportsFP64Imp)(id, SEL) = (BOOL (*)(id, SEL))fp64Imp;
            deviceSupportsFP64 = supportsFP64Imp(gDevice, fp64Selector);
        }

        if (!error && deviceSupportsFP64) {
            NSError *doubleError = nil;
            NSString *doubleSource = [[NSString alloc] initWithUTF8String:kSMAKernelSourceDouble];
            id<MTLLibrary> doubleLibrary = nil;
            if (doubleSource)
                doubleLibrary = [gDevice newLibraryWithSource:doubleSource options:options error:&doubleError];
            [doubleSource release];

            if (doubleLibrary && !doubleError) {
                id<MTLFunction> scanFunctionDouble = [doubleLibrary newFunctionWithName:@"prefix_scan_double"];
                id<MTLFunction> addFunctionDouble = [doubleLibrary newFunctionWithName:@"add_block_offsets_double"];
                id<MTLFunction> smaFunctionDouble = [doubleLibrary newFunctionWithName:@"sma_from_prefix_double"];
                id<MTLFunction> wmaFunctionDouble = [doubleLibrary newFunctionWithName:@"wma_from_prefix_double"];

                if (scanFunctionDouble && addFunctionDouble && smaFunctionDouble && wmaFunctionDouble) {
                    gScanStateDouble = [gDevice newComputePipelineStateWithFunction:scanFunctionDouble error:&doubleError];
                    if (!doubleError && gScanStateDouble)
                        gAddOffsetsStateDouble = [gDevice newComputePipelineStateWithFunction:addFunctionDouble error:&doubleError];
                    if (!doubleError && gAddOffsetsStateDouble)
                        gSMAPrefixStateDouble = [gDevice newComputePipelineStateWithFunction:smaFunctionDouble error:&doubleError];
                    if (!doubleError && gSMAPrefixStateDouble)
                        gWMAStateDouble = [gDevice newComputePipelineStateWithFunction:wmaFunctionDouble error:&doubleError];
                    if (!doubleError && gWMAStateDouble)
                        gSupportsDoublePrecision = true;
                }

                if (scanFunctionDouble) [scanFunctionDouble release];
                if (addFunctionDouble) [addFunctionDouble release];
                if (smaFunctionDouble) [smaFunctionDouble release];
                if (wmaFunctionDouble) [wmaFunctionDouble release];
                [doubleLibrary release];
            }
        }

        [scanFunction release];
        [addFunction release];
        [smaFunction release];
        [wmaFunction release];
        [library release];

        if (!gScanState || !gAddOffsetsState || !gSMAPrefixState || !gWMAState || error) {
            cleanup_resources();
            return false;
        }

        if (!gSupportsDoublePrecision) {
            if (gScanStateDouble) {
                [gScanStateDouble release];
                gScanStateDouble = nil;
            }
            if (gAddOffsetsStateDouble) {
                [gAddOffsetsStateDouble release];
                gAddOffsetsStateDouble = nil;
            }
            if (gSMAPrefixStateDouble) {
                [gSMAPrefixStateDouble release];
                gSMAPrefixStateDouble = nil;
            }
            if (gWMAStateDouble) {
                [gWMAStateDouble release];
                gWMAStateDouble = nil;
            }
        }
    }

    gMetalReady = true;
    return true;
}

template <typename BufferRef>
static bool ensure_buffer_capacity(id<MTLBuffer> &buffer,
                                  NSUInteger &capacity,
                                  NSUInteger requiredCount,
                                  NSUInteger elementSize,
                                  BufferRef &result)
{
    if (requiredCount == 0)
        return true;

    if (buffer && capacity >= requiredCount) {
        result = buffer;
        return true;
    }

    if (buffer) {
        [buffer release];
        buffer = nil;
        capacity = 0;
    }

    buffer = [gDevice newBufferWithLength:elementSize * requiredCount options:MTLResourceStorageModeShared];
    if (!buffer)
        return false;

    capacity = requiredCount;
    result = buffer;
    return true;
}

static bool compute_prefix_from_host(const float *hostValues,
                                     NSUInteger inputLength,
                                     id<MTLBuffer> inputBuffer,
                                     id<MTLBuffer> prefixBuffer,
                                     id<MTLBuffer> blockBuffer)
{
    if (!hostValues || inputLength == 0)
        return false;

    float *inputPtr = static_cast<float *>([inputBuffer contents]);
    if (!inputPtr)
        return false;

    memcpy(inputPtr, hostValues, sizeof(float) * inputLength);

    @autoreleasepool {
        id<MTLCommandBuffer> commandBuffer = [gQueue commandBuffer];
        if (!commandBuffer)
            return false;

        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
        if (!encoder)
            return false;

        uint32_t count32 = static_cast<uint32_t>(inputLength);
        MTLSize tgSize = MTLSizeMake(std::min<NSUInteger>(kThreadgroupSize, gScanState.maxTotalThreadsPerThreadgroup), 1, 1);
        if (tgSize.width == 0)
            tgSize.width = kThreadgroupSize;

        [encoder setComputePipelineState:gScanState];
        [encoder setBuffer:inputBuffer offset:0 atIndex:0];
        [encoder setBuffer:prefixBuffer offset:0 atIndex:1];
        [encoder setBuffer:blockBuffer offset:0 atIndex:2];
        [encoder setBytes:&count32 length:sizeof(count32) atIndex:3];
        [encoder dispatchThreads:MTLSizeMake(inputLength, 1, 1) threadsPerThreadgroup:tgSize];
        [encoder endEncoding];

        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    }

    NSUInteger blockCount = (inputLength + kThreadgroupSize - 1) / kThreadgroupSize;
    if (blockCount == 0)
        blockCount = 1;

    float *blockPtr = static_cast<float *>([blockBuffer contents]);
    if (!blockPtr)
        return false;

    float running = 0.0f;
    for (NSUInteger i = 0; i < blockCount; ++i) {
        float blockSum = blockPtr[i];
        blockPtr[i] = running;
        running += blockSum;
    }

    @autoreleasepool {
        id<MTLCommandBuffer> commandBuffer = [gQueue commandBuffer];
        if (!commandBuffer)
            return false;

        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
        if (!encoder)
            return false;

        uint32_t count32 = static_cast<uint32_t>(inputLength);
        MTLSize tgSizeAdd = MTLSizeMake(std::min<NSUInteger>(kThreadgroupSize, gAddOffsetsState.maxTotalThreadsPerThreadgroup), 1, 1);
        if (tgSizeAdd.width == 0)
            tgSizeAdd.width = kThreadgroupSize;

        [encoder setComputePipelineState:gAddOffsetsState];
        [encoder setBuffer:prefixBuffer offset:0 atIndex:0];
        [encoder setBuffer:blockBuffer offset:0 atIndex:1];
        [encoder setBytes:&count32 length:sizeof(count32) atIndex:2];
        [encoder dispatchThreads:MTLSizeMake(inputLength, 1, 1) threadsPerThreadgroup:tgSizeAdd];
        [encoder endEncoding];

        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    }

    return true;
}

static bool compute_prefix_from_host_double(const double *hostValues,
                                            NSUInteger inputLength,
                                            id<MTLBuffer> inputBuffer,
                                            id<MTLBuffer> prefixBuffer,
                                            id<MTLBuffer> blockBuffer)
{
    if (!gSupportsDoublePrecision)
        return false;

    if (!hostValues || inputLength == 0)
        return false;

    double *inputPtr = static_cast<double *>([inputBuffer contents]);
    if (!inputPtr)
        return false;

    memcpy(inputPtr, hostValues, sizeof(double) * inputLength);

    @autoreleasepool {
        id<MTLCommandBuffer> commandBuffer = [gQueue commandBuffer];
        if (!commandBuffer)
            return false;

        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
        if (!encoder)
            return false;

        uint32_t count32 = static_cast<uint32_t>(inputLength);
        MTLSize tgSize = MTLSizeMake(std::min<NSUInteger>(kThreadgroupSize, gScanStateDouble.maxTotalThreadsPerThreadgroup), 1, 1);
        if (tgSize.width == 0)
            tgSize.width = kThreadgroupSize;

        [encoder setComputePipelineState:gScanStateDouble];
        [encoder setBuffer:inputBuffer offset:0 atIndex:0];
        [encoder setBuffer:prefixBuffer offset:0 atIndex:1];
        [encoder setBuffer:blockBuffer offset:0 atIndex:2];
        [encoder setBytes:&count32 length:sizeof(count32) atIndex:3];
        [encoder dispatchThreads:MTLSizeMake(inputLength, 1, 1) threadsPerThreadgroup:tgSize];
        [encoder endEncoding];

        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    }

    NSUInteger blockCount = (inputLength + kThreadgroupSize - 1) / kThreadgroupSize;
    if (blockCount == 0)
        blockCount = 1;

    double *blockPtr = static_cast<double *>([blockBuffer contents]);
    if (!blockPtr)
        return false;

    double running = 0.0;
    for (NSUInteger i = 0; i < blockCount; ++i) {
        double blockSum = blockPtr[i];
        blockPtr[i] = running;
        running += blockSum;
    }

    @autoreleasepool {
        id<MTLCommandBuffer> commandBuffer = [gQueue commandBuffer];
        if (!commandBuffer)
            return false;

        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
        if (!encoder)
            return false;

        uint32_t count32 = static_cast<uint32_t>(inputLength);
        MTLSize tgSizeAdd = MTLSizeMake(std::min<NSUInteger>(kThreadgroupSize, gAddOffsetsStateDouble.maxTotalThreadsPerThreadgroup), 1, 1);
        if (tgSizeAdd.width == 0)
            tgSizeAdd.width = kThreadgroupSize;

        [encoder setComputePipelineState:gAddOffsetsStateDouble];
        [encoder setBuffer:prefixBuffer offset:0 atIndex:0];
        [encoder setBuffer:blockBuffer offset:0 atIndex:1];
        [encoder setBytes:&count32 length:sizeof(count32) atIndex:2];
        [encoder dispatchThreads:MTLSizeMake(inputLength, 1, 1) threadsPerThreadgroup:tgSizeAdd];
        [encoder endEncoding];

        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    }

    return true;
}

static bool run_wma_kernel_double(const double *inReal,
                                  int inputCount,
                                  int optInTimePeriod,
                                  double *weightedScratch,
                                  double *outReal,
                                  int outputCount)
{
    if (!gSupportsDoublePrecision)
        return false;

    if (!ensure_pipeline())
        return false;

    if (!inReal || !outReal)
        return false;

    if (inputCount <= 0 || outputCount <= 0)
        return false;

    if (optInTimePeriod <= 1)
        return false;

    const NSUInteger inputLength = static_cast<NSUInteger>(inputCount);
    const NSUInteger outputLength = static_cast<NSUInteger>(outputCount);
    const NSUInteger period = static_cast<NSUInteger>(optInTimePeriod);

    id<MTLBuffer> inputBuffer = nil;
    id<MTLBuffer> prefixBuffer = nil;
    id<MTLBuffer> prefixWeightedBuffer = nil;
    id<MTLBuffer> blockBuffer = nil;
    id<MTLBuffer> outputBuffer = nil;

    if (!ensure_buffer_capacity(gInputBufferDouble, gInputCapacityDouble, inputLength, sizeof(double), inputBuffer))
        return false;
    if (!ensure_buffer_capacity(gPrefixBufferDouble, gPrefixCapacityDouble, inputLength, sizeof(double), prefixBuffer))
        return false;
    if (!ensure_buffer_capacity(gPrefixWeightedBufferDouble, gPrefixWeightedCapacityDouble, inputLength, sizeof(double), prefixWeightedBuffer))
        return false;
    const NSUInteger blockCount = (inputLength + kThreadgroupSize - 1) / kThreadgroupSize;
    if (!ensure_buffer_capacity(gBlockBufferDouble, gBlockCapacityDouble, std::max<NSUInteger>(blockCount, 1), sizeof(double), blockBuffer))
        return false;
    if (!ensure_buffer_capacity(gOutputBufferDouble, gOutputCapacityDouble, outputLength, sizeof(double), outputBuffer))
        return false;

    double *outputPtr = static_cast<double *>([outputBuffer contents]);
    if (!outputPtr)
        return false;

    if (!compute_prefix_from_host_double(inReal, inputLength, inputBuffer, prefixBuffer, blockBuffer))
        return false;

    for (NSUInteger i = 0; i < inputLength; ++i)
        weightedScratch[i] = inReal[i] * static_cast<double>(i + 1U);

    if (!compute_prefix_from_host_double(weightedScratch, inputLength, inputBuffer, prefixWeightedBuffer, blockBuffer))
        return false;

    @autoreleasepool {
        id<MTLCommandBuffer> commandBuffer = [gQueue commandBuffer];
        if (!commandBuffer)
            return false;

        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
        if (!encoder)
            return false;

        uint32_t period32 = static_cast<uint32_t>(period);
        uint32_t output32 = static_cast<uint32_t>(outputLength);
        MTLSize tgSizeWMA = MTLSizeMake(std::min<NSUInteger>(kThreadgroupSize, gWMAStateDouble.maxTotalThreadsPerThreadgroup), 1, 1);
        if (tgSizeWMA.width == 0)
            tgSizeWMA.width = kThreadgroupSize;

        [encoder setComputePipelineState:gWMAStateDouble];
        [encoder setBuffer:prefixBuffer offset:0 atIndex:0];
        [encoder setBuffer:prefixWeightedBuffer offset:0 atIndex:1];
        [encoder setBuffer:outputBuffer offset:0 atIndex:2];
        [encoder setBytes:&period32 length:sizeof(period32) atIndex:3];
        [encoder setBytes:&output32 length:sizeof(output32) atIndex:4];
        [encoder dispatchThreads:MTLSizeMake(outputLength, 1, 1) threadsPerThreadgroup:tgSizeWMA];
        [encoder endEncoding];

        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    }

    memcpy(outReal, outputPtr, sizeof(double) * outputLength);
    return true;
}

static bool run_sma_kernel(const float *inReal,
                           int inputCount,
                           int optInTimePeriod,
                           float *outReal,
                           int outputCount)
{
    if (!ensure_pipeline())
        return false;

    if (!inReal || !outReal)
        return false;

    if (inputCount <= 0 || outputCount <= 0)
        return false;

    if (optInTimePeriod <= 1)
        return false;

    const NSUInteger inputLength = static_cast<NSUInteger>(inputCount);
    const NSUInteger outputLength = static_cast<NSUInteger>(outputCount);
    const NSUInteger period = static_cast<NSUInteger>(optInTimePeriod);

    id<MTLBuffer> inputBuffer = nil;
    id<MTLBuffer> prefixBuffer = nil;
    id<MTLBuffer> blockBuffer = nil;
    id<MTLBuffer> outputBuffer = nil;

    if (!ensure_buffer_capacity(gInputBuffer, gInputCapacity, inputLength, sizeof(float), inputBuffer))
        return false;
    if (!ensure_buffer_capacity(gPrefixBuffer, gPrefixCapacity, inputLength, sizeof(float), prefixBuffer))
        return false;
    const NSUInteger blockCount = (inputLength + kThreadgroupSize - 1) / kThreadgroupSize;
    if (!ensure_buffer_capacity(gBlockBuffer, gBlockCapacity, std::max<NSUInteger>(blockCount, 1), sizeof(float), blockBuffer))
        return false;
    if (!ensure_buffer_capacity(gOutputBuffer, gOutputCapacity, outputLength, sizeof(float), outputBuffer))
        return false;

    float *outputPtr = static_cast<float *>([outputBuffer contents]);
    if (!outputPtr)
        return false;

    if (!compute_prefix_from_host(inReal, inputLength, inputBuffer, prefixBuffer, blockBuffer))
        return false;

    @autoreleasepool {
        id<MTLCommandBuffer> commandBuffer = [gQueue commandBuffer];
        if (!commandBuffer)
            return false;

        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
        if (!encoder)
            return false;

        uint32_t period32 = static_cast<uint32_t>(period);
        uint32_t output32 = static_cast<uint32_t>(outputLength);
        MTLSize tgSizeSMA = MTLSizeMake(std::min<NSUInteger>(kThreadgroupSize, gSMAPrefixState.maxTotalThreadsPerThreadgroup), 1, 1);
        if (tgSizeSMA.width == 0)
            tgSizeSMA.width = kThreadgroupSize;

        [encoder setComputePipelineState:gSMAPrefixState];
        [encoder setBuffer:prefixBuffer offset:0 atIndex:0];
        [encoder setBuffer:outputBuffer offset:0 atIndex:1];
        [encoder setBytes:&period32 length:sizeof(period32) atIndex:2];
        [encoder setBytes:&output32 length:sizeof(output32) atIndex:3];
        [encoder dispatchThreads:MTLSizeMake(outputLength, 1, 1) threadsPerThreadgroup:tgSizeSMA];
        [encoder endEncoding];

        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    }

    memcpy(outReal, outputPtr, sizeof(float) * outputLength);
    return true;
}

static bool run_sma_kernel_double(const double *inReal,
                                  int inputCount,
                                  int optInTimePeriod,
                                  double *outReal,
                                  int outputCount)
{
    if (!gSupportsDoublePrecision)
        return false;

    if (!ensure_pipeline())
        return false;

    if (!inReal || !outReal)
        return false;

    if (inputCount <= 0 || outputCount <= 0)
        return false;

    if (optInTimePeriod <= 1)
        return false;

    const NSUInteger inputLength = static_cast<NSUInteger>(inputCount);
    const NSUInteger outputLength = static_cast<NSUInteger>(outputCount);
    const NSUInteger period = static_cast<NSUInteger>(optInTimePeriod);

    id<MTLBuffer> inputBuffer = nil;
    id<MTLBuffer> prefixBuffer = nil;
    id<MTLBuffer> blockBuffer = nil;
    id<MTLBuffer> outputBuffer = nil;

    if (!ensure_buffer_capacity(gInputBufferDouble, gInputCapacityDouble, inputLength, sizeof(double), inputBuffer))
        return false;
    if (!ensure_buffer_capacity(gPrefixBufferDouble, gPrefixCapacityDouble, inputLength, sizeof(double), prefixBuffer))
        return false;
    const NSUInteger blockCount = (inputLength + kThreadgroupSize - 1) / kThreadgroupSize;
    if (!ensure_buffer_capacity(gBlockBufferDouble, gBlockCapacityDouble, std::max<NSUInteger>(blockCount, 1), sizeof(double), blockBuffer))
        return false;
    if (!ensure_buffer_capacity(gOutputBufferDouble, gOutputCapacityDouble, outputLength, sizeof(double), outputBuffer))
        return false;

    double *outputPtr = static_cast<double *>([outputBuffer contents]);
    if (!outputPtr)
        return false;

    if (!compute_prefix_from_host_double(inReal, inputLength, inputBuffer, prefixBuffer, blockBuffer))
        return false;

    @autoreleasepool {
        id<MTLCommandBuffer> commandBuffer = [gQueue commandBuffer];
        if (!commandBuffer)
            return false;

        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
        if (!encoder)
            return false;

        uint32_t period32 = static_cast<uint32_t>(period);
        uint32_t output32 = static_cast<uint32_t>(outputLength);
        MTLSize tgSizeSMA = MTLSizeMake(std::min<NSUInteger>(kThreadgroupSize, gSMAPrefixStateDouble.maxTotalThreadsPerThreadgroup), 1, 1);
        if (tgSizeSMA.width == 0)
            tgSizeSMA.width = kThreadgroupSize;

        [encoder setComputePipelineState:gSMAPrefixStateDouble];
        [encoder setBuffer:prefixBuffer offset:0 atIndex:0];
        [encoder setBuffer:outputBuffer offset:0 atIndex:1];
        [encoder setBytes:&period32 length:sizeof(period32) atIndex:2];
        [encoder setBytes:&output32 length:sizeof(output32) atIndex:3];
        [encoder dispatchThreads:MTLSizeMake(outputLength, 1, 1) threadsPerThreadgroup:tgSizeSMA];
        [encoder endEncoding];

        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    }

    memcpy(outReal, outputPtr, sizeof(double) * outputLength);
    return true;
}

static bool run_wma_kernel(const float *inReal,
                           int inputCount,
                           int optInTimePeriod,
                           float *outReal,
                           int outputCount)
{
    if (!ensure_pipeline())
        return false;

    if (!inReal || !outReal)
        return false;

    if (inputCount <= 0 || outputCount <= 0)
        return false;

    if (optInTimePeriod <= 1)
        return false;

    const NSUInteger inputLength = static_cast<NSUInteger>(inputCount);
    const NSUInteger outputLength = static_cast<NSUInteger>(outputCount);
    const NSUInteger period = static_cast<NSUInteger>(optInTimePeriod);

    id<MTLBuffer> inputBuffer = nil;
    id<MTLBuffer> outputBuffer = nil;

    if (!ensure_buffer_capacity(gInputBuffer, gInputCapacity, inputLength, sizeof(float), inputBuffer))
        return false;
    if (!ensure_buffer_capacity(gOutputBuffer, gOutputCapacity, outputLength, sizeof(float), outputBuffer))
        return false;

    float *outputPtr = static_cast<float *>([outputBuffer contents]);
    if (!outputPtr)
        return false;

    float *inputPtr = static_cast<float *>([inputBuffer contents]);
    if (!inputPtr)
        return false;

    memcpy(inputPtr, inReal, sizeof(float) * inputLength);

    @autoreleasepool {
        id<MTLCommandBuffer> commandBuffer = [gQueue commandBuffer];
        if (!commandBuffer)
            return false;

        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
        if (!encoder)
            return false;

        uint32_t period32 = static_cast<uint32_t>(period);
        uint32_t output32 = static_cast<uint32_t>(outputLength);
        MTLSize tgSizeWMA = MTLSizeMake(std::min<NSUInteger>(kThreadgroupSize, gWMAState.maxTotalThreadsPerThreadgroup), 1, 1);
        if (tgSizeWMA.width == 0)
            tgSizeWMA.width = kThreadgroupSize;

        [encoder setComputePipelineState:gWMAState];
        [encoder setBuffer:inputBuffer offset:0 atIndex:0];
        [encoder setBuffer:outputBuffer offset:0 atIndex:1];
        [encoder setBytes:&period32 length:sizeof(period32) atIndex:2];
        [encoder setBytes:&output32 length:sizeof(output32) atIndex:3];
        [encoder dispatchThreads:MTLSizeMake(outputLength, 1, 1) threadsPerThreadgroup:tgSizeWMA];
        [encoder endEncoding];

        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    }

    memcpy(outReal, outputPtr, sizeof(float) * outputLength);
    return true;
}
} // namespace

extern "C" bool TA_accel_metal_init(void)
{
    return ensure_pipeline();
}

extern "C" void TA_accel_metal_shutdown(void)
{
    cleanup_resources();
}

extern "C" bool TA_accel_metal_sma_double(const double *inReal,
                                           int inputCount,
                                           int optInTimePeriod,
                                           double *outReal,
                                           int outputCount)
{
    if (!inReal || !outReal)
        return false;

    if (gSupportsDoublePrecision)
        return run_sma_kernel_double(inReal, inputCount, optInTimePeriod, outReal, outputCount);

    gScratchDoubleInput.resize(static_cast<size_t>(inputCount));
    gScratchOutput.resize(static_cast<size_t>(outputCount));

    for (int i = 0; i < inputCount; ++i)
        gScratchDoubleInput[static_cast<size_t>(i)] = static_cast<float>(inReal[i]);

    if (!run_sma_kernel(gScratchDoubleInput.data(), inputCount, optInTimePeriod, gScratchOutput.data(), outputCount))
        return false;

    for (int i = 0; i < outputCount; ++i)
        outReal[i] = static_cast<double>(gScratchOutput[static_cast<size_t>(i)]);

    return true;
}

extern "C" bool TA_accel_metal_sma_float(const float *inReal,
                                          int inputCount,
                                          int optInTimePeriod,
                                          double *outReal,
                                          int outputCount)
{
    if (!inReal || !outReal)
        return false;

    gScratchOutput.resize(static_cast<size_t>(outputCount));

    if (!run_sma_kernel(inReal, inputCount, optInTimePeriod, gScratchOutput.data(), outputCount))
        return false;

    for (int i = 0; i < outputCount; ++i)
        outReal[i] = static_cast<double>(gScratchOutput[static_cast<size_t>(i)]);

    return true;
}

extern "C" bool TA_accel_metal_wma_double(const double *inReal,
                                           int inputCount,
                                           int optInTimePeriod,
                                           double *outReal,
                                           int outputCount)
{
    if (!inReal || !outReal)
        return false;

    if (inputCount <= 0 || outputCount <= 0)
        return false;

    if (!gSupportsDoublePrecision)
        return false;

    if (gSupportsDoublePrecision) {
        gScratchWeightedDouble.resize(static_cast<size_t>(inputCount));
        return run_wma_kernel_double(inReal,
                                     inputCount,
                                     optInTimePeriod,
                                     gScratchWeightedDouble.data(),
                                     outReal,
                                     outputCount);
    }

    return false;
}
