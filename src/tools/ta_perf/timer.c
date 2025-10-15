/* High-resolution timer - Cross-platform microsecond timing */

#include "ta_perf.h"

#ifdef _WIN32
    #include <windows.h>
    static double qpc_to_us = 0.0;
#elif defined(__APPLE__) && defined(__MACH__)
    #include <mach/mach_time.h>
    static double mach_to_us = 0.0;
#elif defined(CLOCK_MONOTONIC)
    #include <time.h>
#else
    #include <time.h>
    static double clock_to_us = 0.0;
#endif

void timer_init(void) {
#ifdef _WIN32
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    qpc_to_us = 1000000.0 / (double)freq.QuadPart;
#elif defined(__APPLE__) && defined(__MACH__)
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    mach_to_us = (double)timebase.numer / ((double)timebase.denom * 1000.0);
#elif !defined(CLOCK_MONOTONIC)
    clock_to_us = 1000000.0 / (double)CLOCKS_PER_SEC;
#endif
}

uint64_t timer_now_us(void) {
#ifdef _WIN32
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (uint64_t)(counter.QuadPart * qpc_to_us);
#elif defined(__APPLE__) && defined(__MACH__)
    return (uint64_t)(mach_absolute_time() * mach_to_us);
#elif defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
#else
    return (uint64_t)(clock() * clock_to_us);
#endif
}

double timer_elapsed_us(uint64_t start, uint64_t end) {
    return (double)(end - start);
}
