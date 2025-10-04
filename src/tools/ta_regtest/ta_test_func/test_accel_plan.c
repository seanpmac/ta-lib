/* Tests for multi-indicator acceleration plan batching. */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_accel_plan.h"
#include "ta_func.h"
#include "ta_memory.h"

static const unsigned int kTimeframes[] = { 1U, 60U, 300U, 900U, 3600U };
static const unsigned int kIndicatorCount = 5U;
static const TA_AccelIndicatorType kIndicatorTypes[5] = {
    TA_ACCEL_INDICATOR_SMA,
    TA_ACCEL_INDICATOR_EMA,
    TA_ACCEL_INDICATOR_RSI,
    TA_ACCEL_INDICATOR_TRIMA,
    TA_ACCEL_INDICATOR_WMA
};
static const unsigned int kIndicatorPeriods[5] = { 30U, 26U, 14U, 25U, 20U };

static void build_sequence(double *buffer, unsigned int length)
{
    for (unsigned int i = 0; i < length; ++i)
        buffer[i] = (double)(i + 1);
}

static double *aggregate_timeframe(const double *seconds,
                                   size_t secondsCount,
                                   unsigned int frameSec,
                                   int *outLength)
{
    if (frameSec == 1U) {
        double *copy = (double *)TA_Malloc(sizeof(double) * secondsCount);
        if (!copy)
            return NULL;
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
    if (!dest)
        return NULL;

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

static ErrorNumber compare_with_talib(const double *input,
                                      int length,
                                      TA_AccelIndicatorType type,
                                      unsigned int period,
                                      const TA_AccelIndicatorOutput *out)
{
    int lookback;
    TA_RetCode rc = TA_SUCCESS;
    int outBegIdx = 0;
    int outNbElement = 0;

    switch (type) {
    case TA_ACCEL_INDICATOR_SMA:
        lookback = TA_SMA_Lookback((int)period);
        break;
    case TA_ACCEL_INDICATOR_EMA:
        lookback = TA_EMA_Lookback((int)period);
        break;
    case TA_ACCEL_INDICATOR_RSI:
        lookback = TA_RSI_Lookback((int)period);
        break;
    case TA_ACCEL_INDICATOR_TRIMA:
        lookback = TA_TRIMA_Lookback((int)period);
        break;
    case TA_ACCEL_INDICATOR_WMA:
        lookback = TA_WMA_Lookback((int)period);
        break;
    default:
        return TA_TESTUTIL_TFRR_BAD_PARAM;
    }

    if (length <= lookback) {
        return (out->outNBElement == 0) ? TA_TEST_PASS : TA_TESTUTIL_TFRR_BAD_BEGIDX;
    }

    int startIdx = lookback;
    int endIdx = length - 1;
    double *ref = (double *)TA_Malloc(sizeof(double) * (size_t)(endIdx - startIdx + 1));
    if (!ref)
        return TA_TESTUTIL_DRT_ALLOC_ERR;

    switch (type) {
    case TA_ACCEL_INDICATOR_SMA:
        rc = TA_SMA(startIdx, endIdx, input, (int)period, &outBegIdx, &outNbElement, ref);
        break;
    case TA_ACCEL_INDICATOR_EMA:
        rc = TA_EMA(startIdx, endIdx, input, (int)period, &outBegIdx, &outNbElement, ref);
        break;
    case TA_ACCEL_INDICATOR_RSI:
        rc = TA_RSI(startIdx, endIdx, input, (int)period, &outBegIdx, &outNbElement, ref);
        break;
    case TA_ACCEL_INDICATOR_TRIMA:
        rc = TA_TRIMA(startIdx, endIdx, input, (int)period, &outBegIdx, &outNbElement, ref);
        break;
    case TA_ACCEL_INDICATOR_WMA:
        rc = TA_WMA(startIdx, endIdx, input, (int)period, &outBegIdx, &outNbElement, ref);
        break;
    default:
        rc = TA_BAD_PARAM;
        break;
    }

    if (rc != TA_SUCCESS) {
        TA_Free(ref);
        return TA_TESTUTIL_TFRR_BAD_RETCODE;
    }

    if (out->outBegIdx != outBegIdx || out->outNBElement != outNbElement) {
        TA_Free(ref);
        return TA_TESTUTIL_TFRR_BAD_BEGIDX;
    }

    for (int i = 0; i < outNbElement; ++i) {
        double diff = fabs(out->outReal[i] - ref[i]);
        if (diff > 1e-8) {
            TA_Free(ref);
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
        }
    }

    TA_Free(ref);
    return TA_TEST_PASS;
}

ErrorNumber test_accel_plan(TA_History *history)
{
    (void)history;
    const size_t secondsCount = 7200U; /* two hours of 1-second bars */
    double *seconds = (double *)TA_Malloc(sizeof(double) * secondsCount);
    if (!seconds)
        return TA_TESTUTIL_DRT_ALLOC_ERR;
    build_sequence(seconds, (unsigned int)secondsCount);

    double *timeframeData[sizeof(kTimeframes)/sizeof(kTimeframes[0])];
    int timeframeLen[sizeof(kTimeframes)/sizeof(kTimeframes[0])];

    for (unsigned int i = 0; i < sizeof(kTimeframes)/sizeof(kTimeframes[0]); ++i) {
        timeframeData[i] = aggregate_timeframe(seconds, secondsCount, kTimeframes[i], &timeframeLen[i]);
        if (kTimeframes[i] == 1U && !timeframeData[i]) {
            TA_Free(seconds);
            return TA_TESTUTIL_DRT_ALLOC_ERR;
        }
        if (kTimeframes[i] != 1U && timeframeLen[i] == 0) {
            timeframeData[i] = NULL;
        }
    }

    TA_AccelIndicatorOutput outputs[sizeof(kTimeframes)/sizeof(kTimeframes[0])][kIndicatorCount];
    double *buffers[sizeof(kTimeframes)/sizeof(kTimeframes[0])][kIndicatorCount];

    memset(buffers, 0, sizeof(buffers));

    TA_AccelIndicatorRequest requests[sizeof(kTimeframes)/sizeof(kTimeframes[0]) * kIndicatorCount];
    unsigned int requestCount = 0;

    for (unsigned int tf = 0; tf < sizeof(kTimeframes)/sizeof(kTimeframes[0]); ++tf) {
        const double *data = timeframeData[tf];
        int len = timeframeLen[tf];
        unsigned int frameSec = kTimeframes[tf];
        if (!data || len <= 0)
            continue;

        for (unsigned int ind = 0; ind < kIndicatorCount; ++ind) {
            unsigned int period = kIndicatorPeriods[ind];
            int lookback;
            switch (kIndicatorTypes[ind]) {
            case TA_ACCEL_INDICATOR_SMA:
                lookback = (int)period - 1;
                break;
            case TA_ACCEL_INDICATOR_EMA:
                lookback = (int)period - 1;
                break;
            case TA_ACCEL_INDICATOR_RSI:
                lookback = (int)period;
                break;
            case TA_ACCEL_INDICATOR_TRIMA:
                lookback = (int)period - 1;
                break;
            case TA_ACCEL_INDICATOR_WMA:
                lookback = (int)period - 1;
                break;
            default:
                lookback = 0;
                break;
            }
            size_t outLen = (len > lookback) ? (size_t)(len - lookback) : 1U;
            buffers[tf][ind] = (double *)TA_Malloc(sizeof(double) * outLen);
            if (!buffers[tf][ind]) {
                for (unsigned int t = 0; t <= tf; ++t) {
                    for (unsigned int j = 0; j < kIndicatorCount; ++j)
                        TA_Free(buffers[t][j]);
                }
                for (unsigned int i = 0; i < sizeof(kTimeframes)/sizeof(kTimeframes[0]); ++i)
                    TA_Free(timeframeData[i]);
                TA_Free(seconds);
                return TA_TESTUTIL_DRT_ALLOC_ERR;
            }
            outputs[tf][ind] = (TA_AccelIndicatorOutput){ period, buffers[tf][ind], 0, 0 };
            requests[requestCount++] = (TA_AccelIndicatorRequest){
                kIndicatorTypes[ind], frameSec, data, len, &outputs[tf][ind], 1U
            };
        }
    }

    TA_RetCode rc = TA_accel_execute_plan(requests, requestCount);
    if (rc != TA_SUCCESS) {
        for (unsigned int tf = 0; tf < sizeof(kTimeframes)/sizeof(kTimeframes[0]); ++tf)
            for (unsigned int ind = 0; ind < kIndicatorCount; ++ind)
                TA_Free(buffers[tf][ind]);
        for (unsigned int i = 0; i < sizeof(kTimeframes)/sizeof(kTimeframes[0]); ++i)
            TA_Free(timeframeData[i]);
        TA_Free(seconds);
        return TA_TESTUTIL_TFRR_BAD_RETCODE;
    }

    ErrorNumber err = TA_TEST_PASS;
    for (unsigned int tf = 0; tf < sizeof(kTimeframes)/sizeof(kTimeframes[0]) && err == TA_TEST_PASS; ++tf) {
        const double *data = timeframeData[tf];
        int len = timeframeLen[tf];
        if (!data || len <= 0)
            continue;
        for (unsigned int ind = 0; ind < kIndicatorCount; ++ind) {
            err = compare_with_talib(data, len, kIndicatorTypes[ind], kIndicatorPeriods[ind], &outputs[tf][ind]);
            if (err != TA_TEST_PASS)
                break;
        }
    }

    for (unsigned int tf = 0; tf < sizeof(kTimeframes)/sizeof(kTimeframes[0]); ++tf)
        for (unsigned int ind = 0; ind < kIndicatorCount; ++ind)
            TA_Free(buffers[tf][ind]);
    for (unsigned int i = 0; i < sizeof(kTimeframes)/sizeof(kTimeframes[0]); ++i)
        TA_Free(timeframeData[i]);
    TA_Free(seconds);

    return err;
}
