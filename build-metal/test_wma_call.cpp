#include <cstdio>
#include <vector>
extern "C" {
#include "ta_common.h"
#include "ta_accel.h"
}

int main() {
    TA_accel_init();
    bool ready = TA_accel_is_ready();
    printf("accel ready: %d\n", ready ? 1 : 0);
    const int length = 10;
    double data[length];
    for (int i = 0; i < length; ++i) data[i] = 100.0 + i;
    double out[length];
    bool ok = TA_accel_wma_double(data, 3, 2, length - 1, 2, out);
    printf("TA_accel_wma_double returned %d\n", ok ? 1 : 0);
    if (ok) {
        printf("out[0]=%.10f\n", out[0]);
    }
    TA_accel_shutdown();
    return 0;
}
