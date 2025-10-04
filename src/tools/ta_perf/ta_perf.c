/*
 * TA-Lib performance micro-benchmark for SMA acceleration.
 *
 * Measures CPU loop, TA_SMA API, and (when available) the accelerator helper.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <inttypes.h>

#include "ta_libc.h"
#include "ta_accel.h"
#include "ta_accel_plan.h"
#include "ta_memory.h"

#if defined(__APPLE__)
#include <mach/mach_time.h>
static double monotonic_seconds(void)
{
    static mach_timebase_info_data_t timebase;
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }
    uint64_t now = mach_absolute_time();
    double nanos = (double)now * (double)timebase.numer / (double)timebase.denom;
    return nanos * 1e-9;
}
#else
#include <time.h>
static double monotonic_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}
#endif

static void print_usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [--length=N] [--period=P] [--iterations=I]\n"
            "  --length=N     Number of samples to generate (default 1000000)\n"
            "  --period=P     SMA period (default 30)\n"
            "  --iterations=I Number of timing repetitions per path (default 10)\n",
            prog);
}

static void compute_sma_cpu(const TA_Real *inReal,
                            TA_Integer optInTimePeriod,
                            TA_Integer startIdx,
                            TA_Integer endIdx,
                            TA_Real *outReal)
{
    const TA_Integer lookback = optInTimePeriod - 1;
    TA_Integer trailingIdx = startIdx - lookback;
    TA_Integer outIdx = 0;
    double periodTotal = 0.0;
    TA_Integer i;

    for (i = trailingIdx; i < startIdx; ++i)
        periodTotal += inReal[i];

    for (i = startIdx; i <= endIdx; ++i) {
        periodTotal += inReal[i];
        outReal[outIdx++] = (TA_Real)(periodTotal / optInTimePeriod);
        periodTotal -= inReal[trailingIdx++];
    }
}

static void fill_series(TA_Real *buffer, size_t length)
{
    /* Deterministic waveform with gentle variation */
    const double scale = 100.0;
    for (size_t i = 0; i < length; ++i) {
        double t = (double)i / (double)length;
        buffer[i] = (TA_Real)(scale * (sin(20.0 * t) + 0.5 * cos(3.0 * t)) + 100.0);
    }
}

static double *aggregate_series(const double *seconds,
                                size_t secondsCount,
                                unsigned int frameSec,
                                int *outLength)
{
    if (!seconds || !outLength || frameSec == 0U)
        return NULL;

    if (frameSec == 1U) {
        double *copy = (double *)TA_Malloc(sizeof(double) * secondsCount);
        if (!copy) {
            *outLength = 0;
            return NULL;
        }
        memcpy(copy, seconds, sizeof(double) * secondsCount);
        *outLength = (int)secondsCount;
        return copy;
    }

    size_t groupCount = secondsCount / frameSec;
    if (groupCount == 0) {
        *outLength = 0;
        return NULL;
    }

    double *dest = (double *)TA_Malloc(sizeof(double) * groupCount);
    if (!dest) {
        *outLength = 0;
        return NULL;
    }

    for (size_t g = 0; g < groupCount; ++g) {
        double sum = 0.0;
        size_t base = g * frameSec;
        for (unsigned int i = 0; i < frameSec; ++i)
            sum += seconds[base + i];
        dest[g] = sum / (double)frameSec;
    }

    *outLength = (int)groupCount;
    return dest;
}

static int indicator_lookback(TA_AccelIndicatorType type, unsigned int period)
{
    switch (type) {
    case TA_ACCEL_INDICATOR_SMA:
        return TA_SMA_Lookback((int)period);
    case TA_ACCEL_INDICATOR_EMA:
        return TA_EMA_Lookback((int)period);
    case TA_ACCEL_INDICATOR_RSI:
        return TA_RSI_Lookback((int)period);
    case TA_ACCEL_INDICATOR_TRIMA:
        return TA_TRIMA_Lookback((int)period);
    case TA_ACCEL_INDICATOR_WMA:
        return TA_WMA_Lookback((int)period);
    default:
        return 0;
    }
}

static int benchmark_indicator_plan(const double *seconds,
                                    size_t secondsCount,
                                    unsigned int iterations);

int main(int argc, char **argv)
{
    size_t length = 1000000U;
    TA_Integer period = 30;
    unsigned int iterations = 10;

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (strncmp(arg, "--length=", 9) == 0) {
            length = (size_t)strtoull(arg + 9, NULL, 10);
        } else if (strncmp(arg, "--period=", 9) == 0) {
            period = (TA_Integer)strtol(arg + 9, NULL, 10);
        } else if (strncmp(arg, "--iterations=", 13) == 0) {
            iterations = (unsigned int)strtoul(arg + 13, NULL, 10);
        } else {
            fprintf(stderr, "Unknown option: %s\n", arg);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (period < 2) {
        fprintf(stderr, "Period must be >= 2\n");
        return EXIT_FAILURE;
    }

    if (length < (size_t)period) {
        fprintf(stderr, "Length must be >= period\n");
        return EXIT_FAILURE;
    }

    if (iterations == 0) {
        fprintf(stderr, "Iterations must be >= 1\n");
        return EXIT_FAILURE;
    }

    TA_RetCode retCode = TA_Initialize();
    if (retCode != TA_SUCCESS) {
        fprintf(stderr, "TA_Initialize failed: %d\n", retCode);
        return EXIT_FAILURE;
    }

    TA_Integer startIdx = period - 1;
    TA_Integer endIdx = (TA_Integer)length - 1;
    TA_Integer lookback = period - 1;
    TA_Integer expectedCount = endIdx - startIdx + 1;

    TA_Real *input = (TA_Real *)TA_Malloc(sizeof(TA_Real) * length);
    TA_Real *cpuOut = (TA_Real *)TA_Malloc(sizeof(TA_Real) * (size_t)expectedCount);
    TA_Real *libOut = (TA_Real *)TA_Malloc(sizeof(TA_Real) * (size_t)expectedCount);
    TA_Real *accelOut = (TA_Real *)TA_Malloc(sizeof(TA_Real) * (size_t)expectedCount);

    if (!input || !cpuOut || !libOut || !accelOut) {
        fprintf(stderr, "Allocation failure\n");
        TA_Free(input);
        TA_Free(cpuOut);
        TA_Free(libOut);
        TA_Free(accelOut);
        TA_Shutdown();
        return EXIT_FAILURE;
    }

    fill_series(input, length);

    /* Warm-up */
    compute_sma_cpu(input, period, startIdx, endIdx, cpuOut);
    TA_Integer outBegIdx = 0;
    TA_Integer outNbElement = 0;
    retCode = TA_SMA(startIdx, endIdx, input, period, &outBegIdx, &outNbElement, libOut);
    if (retCode != TA_SUCCESS) {
        fprintf(stderr, "TA_SMA warm-up failed: %d\n", retCode);
        TA_Free(input);
        TA_Free(cpuOut);
        TA_Free(libOut);
        TA_Free(accelOut);
        TA_Shutdown();
        return EXIT_FAILURE;
    }

    if (outBegIdx != startIdx || outNbElement != expectedCount) {
        fprintf(stderr, "Unexpected TA_SMA output shape (beg=%d nb=%d)\n", outBegIdx, outNbElement);
        TA_Free(input);
        TA_Free(cpuOut);
        TA_Free(libOut);
        TA_Free(accelOut);
        TA_Shutdown();
        return EXIT_FAILURE;
    }

    bool accelReady = TA_accel_is_ready();
    if (accelReady) {
        if (!TA_accel_sma_double(input, period, startIdx, endIdx, lookback, accelOut)) {
            fprintf(stderr, "Accelerator reported ready but SMA call failed; disabling acceleration path.\n");
            accelReady = false;
        }
    }

    double cpuTotal = 0.0;
    double libTotal = 0.0;
    double accelTotal = 0.0;

    for (unsigned int iter = 0; iter < iterations; ++iter) {
        double t0 = monotonic_seconds();
        compute_sma_cpu(input, period, startIdx, endIdx, cpuOut);
        double t1 = monotonic_seconds();
        cpuTotal += (t1 - t0);

        t0 = monotonic_seconds();
        retCode = TA_SMA(startIdx, endIdx, input, period, &outBegIdx, &outNbElement, libOut);
        double t2 = monotonic_seconds();
        if (retCode != TA_SUCCESS) {
            fprintf(stderr, "TA_SMA iteration failed: %d\n", retCode);
            TA_Free(input);
            TA_Free(cpuOut);
            TA_Free(libOut);
            TA_Free(accelOut);
            TA_Shutdown();
            return EXIT_FAILURE;
        }
        libTotal += (t2 - t0);

        if (accelReady) {
            t0 = monotonic_seconds();
            if (!TA_accel_sma_double(input, period, startIdx, endIdx, lookback, accelOut)) {
                fprintf(stderr, "Accelerated SMA failed during iteration; aborting.\n");
                TA_Free(input);
                TA_Free(cpuOut);
                TA_Free(libOut);
                TA_Free(accelOut);
                TA_Shutdown();
                return EXIT_FAILURE;
            }
            t2 = monotonic_seconds();
            accelTotal += (t2 - t0);
        }
    }

    double cpuAvg = cpuTotal / (double)iterations * 1e3;   /* milliseconds */
    double libAvg = libTotal / (double)iterations * 1e3;
    double accelAvg = accelReady ? (accelTotal / (double)iterations * 1e3) : 0.0;

    /* Validate numerical parity */
    double maxDiffLib = 0.0;
    double maxDiffAccel = 0.0;
    for (TA_Integer i = 0; i < expectedCount; ++i) {
        double diffLib = fabs((double)libOut[i] - (double)cpuOut[i]);
        if (diffLib > maxDiffLib)
            maxDiffLib = diffLib;
        if (accelReady) {
            double diffAccel = fabs((double)accelOut[i] - (double)cpuOut[i]);
            if (diffAccel > maxDiffAccel)
                maxDiffAccel = diffAccel;
        }
    }

    printf("TA-Lib SMA performance benchmark\n");
    printf("Samples        : %zu\n", length);
    printf("Period         : %d\n", period);
    printf("Iterations     : %u\n", iterations);
    printf("Acceleration   : %s\n", accelReady ? "enabled" : "disabled/not available");
    printf("CPU loop       : %.3f ms/iter\n", cpuAvg);
    printf("TA_SMA API     : %.3f ms/iter (max diff vs CPU %.3g)\n", libAvg, maxDiffLib);
    if (accelReady) {
        printf("TA_accel SMA   : %.3f ms/iter (max diff vs CPU %.3g)\n", accelAvg, maxDiffAccel);
        if (accelAvg > 0.0)
            printf("Speedup (CPU/accel): %.2fx\n", cpuAvg / accelAvg);
    }

    if (benchmark_indicator_plan(input, length, iterations) != 0) {
        TA_Free(input);
        TA_Free(cpuOut);
        TA_Free(libOut);
        TA_Free(accelOut);
        TA_Shutdown();
        return EXIT_FAILURE;
    }

    TA_Free(input);
    TA_Free(cpuOut);
    TA_Free(libOut);
    TA_Free(accelOut);
    TA_Shutdown();
    return EXIT_SUCCESS;
}

static int benchmark_indicator_plan(const double *seconds,
                                    size_t secondsCount,
                                    unsigned int iterations)
{
    if (!seconds || secondsCount == 0)
        return -1;

    enum { TIMEFRAME_COUNT = 5, INDICATOR_COUNT = 5 };
    static const unsigned int TIMEFRAMES[TIMEFRAME_COUNT] = { 1U, 60U, 300U, 900U, 3600U };
    static const struct {
        TA_AccelIndicatorType type;
        unsigned int period;
    } INDICATORS[INDICATOR_COUNT] = {
        { TA_ACCEL_INDICATOR_SMA,   30U },
        { TA_ACCEL_INDICATOR_EMA,   26U },
        { TA_ACCEL_INDICATOR_RSI,   14U },
        { TA_ACCEL_INDICATOR_TRIMA, 25U },
        { TA_ACCEL_INDICATOR_WMA,   20U }
    };

    double *timeframeData[TIMEFRAME_COUNT];
    int timeframeLen[TIMEFRAME_COUNT];
    bool timeframeAllocated[TIMEFRAME_COUNT];

    memset(timeframeData, 0, sizeof(timeframeData));
    memset(timeframeLen, 0, sizeof(timeframeLen));
    memset(timeframeAllocated, 0, sizeof(timeframeAllocated));

    for (unsigned int tf = 0; tf < TIMEFRAME_COUNT; ++tf) {
        timeframeData[tf] = aggregate_series(seconds, secondsCount, TIMEFRAMES[tf], &timeframeLen[tf]);
        if (!timeframeData[tf]) {
            if (timeframeLen[tf] == 0)
                continue; /* Not enough data, skip timeframe */
            goto cleanup;
        }
        timeframeAllocated[tf] = true;
    }

    TA_AccelIndicatorOutput accelOutputs[TIMEFRAME_COUNT][INDICATOR_COUNT];
    TA_AccelIndicatorOutput cpuOutputs[TIMEFRAME_COUNT][INDICATOR_COUNT];
    double *accelBuffers[TIMEFRAME_COUNT][INDICATOR_COUNT];
    double *cpuBuffers[TIMEFRAME_COUNT][INDICATOR_COUNT];
    memset(accelBuffers, 0, sizeof(accelBuffers));
    memset(cpuBuffers, 0, sizeof(cpuBuffers));

    TA_AccelIndicatorRequest accelRequests[TIMEFRAME_COUNT * INDICATOR_COUNT];
    TA_AccelIndicatorRequest cpuRequests[TIMEFRAME_COUNT * INDICATOR_COUNT];
    unsigned int requestCount = 0;

    for (unsigned int tf = 0; tf < TIMEFRAME_COUNT; ++tf) {
        const double *data = timeframeData[tf];
        int len = timeframeLen[tf];
        if (!data || len <= 0)
            continue;

        unsigned int frameSec = TIMEFRAMES[tf];
        for (unsigned int ind = 0; ind < INDICATOR_COUNT; ++ind) {
            unsigned int period = INDICATORS[ind].period;
            int lookback = indicator_lookback(INDICATORS[ind].type, period);
            size_t outLen = (len > lookback) ? (size_t)(len - lookback) : 0;
            size_t allocLen = (outLen > 0) ? outLen : 1U;

            double *accBuf = (double *)TA_Malloc(sizeof(double) * allocLen);
            double *cpuBuf = (double *)TA_Malloc(sizeof(double) * allocLen);
            if (!accBuf || !cpuBuf) {
                TA_Free(accBuf);
                TA_Free(cpuBuf);
                goto cleanup;
            }

            accelBuffers[tf][ind] = accBuf;
            cpuBuffers[tf][ind] = cpuBuf;
            accelOutputs[tf][ind] = (TA_AccelIndicatorOutput){ period, accBuf, 0, 0 };
            cpuOutputs[tf][ind] = (TA_AccelIndicatorOutput){ period, cpuBuf, 0, 0 };

            accelRequests[requestCount] = (TA_AccelIndicatorRequest){
                INDICATORS[ind].type, frameSec, data, len, &accelOutputs[tf][ind], 1U
            };
            cpuRequests[requestCount] = (TA_AccelIndicatorRequest){
                INDICATORS[ind].type, frameSec, data, len, &cpuOutputs[tf][ind], 1U
            };
            ++requestCount;
        }
    }

    if (requestCount == 0) {
        printf("\nMulti-indicator plan benchmark skipped (insufficient data).\n");
        goto cleanup_success;
    }

    bool accelInitialized = false;
    double accelTotal = 0.0;
    double cpuTotal = 0.0;
    double maxDiff = 0.0;
    size_t totalSignals = 0;
    unsigned int usedTimeframes = 0;

    for (unsigned int tf = 0; tf < TIMEFRAME_COUNT; ++tf)
        if (timeframeData[tf] && timeframeLen[tf] > 0)
            ++usedTimeframes;

    TA_accel_init();
    accelInitialized = true;
    bool accelAvailable = TA_accel_is_ready();
    unsigned int accelIterations = accelAvailable ? iterations : 1U;

    for (unsigned int iter = 0; iter < accelIterations; ++iter) {
        double t0 = monotonic_seconds();
        TA_RetCode rc = TA_accel_execute_plan(accelRequests, requestCount);
        double t1 = monotonic_seconds();
        if (rc != TA_SUCCESS) {
            fprintf(stderr, "TA_accel_execute_plan failed (accelerated path): %d\n", rc);
            goto cleanup;
        }
        accelTotal += (t1 - t0);
    }
    if (accelAvailable)
        accelTotal = (accelTotal / (double)accelIterations) * 1e3;
    else
        accelTotal = 0.0;

    TA_accel_shutdown();
    accelInitialized = false;

    for (unsigned int iter = 0; iter < iterations; ++iter) {
        double t0 = monotonic_seconds();
        TA_RetCode rc = TA_accel_execute_plan(cpuRequests, requestCount);
        double t1 = monotonic_seconds();
        if (rc != TA_SUCCESS) {
            fprintf(stderr, "TA_accel_execute_plan failed (CPU path): %d\n", rc);
            goto cleanup;
        }
        cpuTotal += (t1 - t0);
    }
    cpuTotal = (cpuTotal / (double)iterations) * 1e3;

    for (unsigned int r = 0; r < requestCount; ++r) {
        const TA_AccelIndicatorOutput *accOut = accelRequests[r].outputs;
        const TA_AccelIndicatorOutput *cpuOut = cpuRequests[r].outputs;
        if (!accOut || !cpuOut)
            continue;
        if (accOut->outBegIdx != cpuOut->outBegIdx || accOut->outNBElement != cpuOut->outNBElement) {
            fprintf(stderr, "Output shape mismatch for indicator %u timeframe %u period %u\n",
                    accelRequests[r].type, accelRequests[r].timeframeSeconds, accOut->period);
            goto cleanup;
        }
        if (cpuOut->outNBElement > 0)
            totalSignals += (size_t)cpuOut->outNBElement;
        for (int k = 0; k < cpuOut->outNBElement; ++k) {
            double diff = fabs(accOut->outReal[k] - cpuOut->outReal[k]);
            if (diff > maxDiff)
                maxDiff = diff;
        }
    }

    printf("\nMulti-indicator plan benchmark (SMA/EMA/RSI/TRIMA/WMA)\n");
    printf("Timeframes      : %u\n", usedTimeframes);
    printf("Signals computed: %zu\n", totalSignals);
    printf("Acceleration    : %s\n", accelAvailable ? "enabled" : "disabled/not available");
    if (accelAvailable)
        printf("Accelerated plan: %.3f ms/iter\n", accelTotal);
    else
        printf("Accelerated plan: n/a\n");
    printf("CPU plan        : %.3f ms/iter\n", cpuTotal);
    if (accelAvailable && accelTotal > 0.0)
        printf("Speedup (CPU/accel): %.2fx\n", cpuTotal / accelTotal);
    printf("Max diff vs CPU : %.3g\n", maxDiff);

cleanup_success:
    for (unsigned int tf = 0; tf < TIMEFRAME_COUNT; ++tf) {
        for (unsigned int ind = 0; ind < INDICATOR_COUNT; ++ind) {
            TA_Free(accelBuffers[tf][ind]);
            TA_Free(cpuBuffers[tf][ind]);
        }
    }
    for (unsigned int tf = 0; tf < TIMEFRAME_COUNT; ++tf)
        if (timeframeAllocated[tf] && timeframeData[tf])
            TA_Free(timeframeData[tf]);
    return 0;

cleanup:
    if (accelInitialized)
        TA_accel_shutdown();
    for (unsigned int tf = 0; tf < TIMEFRAME_COUNT; ++tf) {
        for (unsigned int ind = 0; ind < INDICATOR_COUNT; ++ind) {
            TA_Free(accelBuffers[tf][ind]);
            TA_Free(cpuBuffers[tf][ind]);
        }
    }
    for (unsigned int tf = 0; tf < TIMEFRAME_COUNT; ++tf)
        if (timeframeAllocated[tf] && timeframeData[tf])
            TA_Free(timeframeData[tf]);
    return -1;
}
