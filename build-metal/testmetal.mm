#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

int main() {
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            printf("No Metal device\n");
            return 1;
        }
        BOOL supportsFP64 = NO;
        if ([device respondsToSelector:@selector(supports64BitFloat)]) {
            supportsFP64 = [device supports64BitFloat];
        }
        NSLog(@"Device: %@, supports64BitFloat=%@", device.name, supportsFP64 ? @"YES" : @"NO");
    }
    return 0;
}
