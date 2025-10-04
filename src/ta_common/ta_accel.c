#include "ta_accel.h"
#include "ta_memory.h"

#include <stddef.h>

#if defined(TA_ACCEL_HAS_METAL)
/* Backend entry points implemented in ta_accel_metal.mm */
bool TA_accel_metal_init(void);
void TA_accel_metal_shutdown(void);
bool TA_accel_metal_sma_double(const double *inReal,
                               int inputCount,
                               int optInTimePeriod,
                               double *outReal,
                               int outputCount);
bool TA_accel_metal_sma_float(const float *inReal,
                              int inputCount,
                              int optInTimePeriod,
                              double *outReal,
                              int outputCount);
bool TA_accel_metal_wma_double(const double *inReal,
                               int inputCount,
                               int optInTimePeriod,
                               double *outReal,
                               int outputCount);
#endif

static bool accel_ready = false;

void TA_accel_init(void)
{
#if defined(TA_ACCEL_HAS_METAL)
    accel_ready = TA_accel_metal_init();
#else
    accel_ready = false;
#endif
}

void TA_accel_shutdown(void)
{
#if defined(TA_ACCEL_HAS_METAL)
    if (accel_ready)
        TA_accel_metal_shutdown();
#endif
    accel_ready = false;
}

bool TA_accel_is_ready(void)
{
    return accel_ready;
}

static bool validate_window(int optInTimePeriod,
                            int startIdx,
                            int endIdx,
                            int lookbackTotal,
                            int *trailingIdx,
                            int *inputCount,
                            int *outputCount)
{
    if (!accel_ready)
        return false;
    if (optInTimePeriod <= 1)
        return false;
    if (startIdx > endIdx)
        return false;

    int lookback = lookbackTotal;
    if (lookback < 0)
        return false;

    int trailing = startIdx - lookback;
    if (trailing < 0)
        return false;

    int inputs = endIdx - trailing + 1;
    int outputs = endIdx - startIdx + 1;

    if (inputs <= 0 || outputs <= 0)
        return false;

    if (optInTimePeriod > inputs)
        return false;

    if (trailingIdx)
        *trailingIdx = trailing;
    if (inputCount)
        *inputCount = inputs;
    if (outputCount)
        *outputCount = outputs;

    return true;
}

bool TA_accel_sma_double(const double *inReal,
                         int optInTimePeriod,
                         int startIdx,
                         int endIdx,
                         int lookbackTotal,
                         double *outReal)
{
    int trailingIdx = 0;
    int inputCount = 0;
    int outputCount = 0;

    if (!inReal || !outReal)
        return false;

    if (!validate_window(optInTimePeriod,
                         startIdx,
                         endIdx,
                         lookbackTotal,
                         &trailingIdx,
                         &inputCount,
                         &outputCount))
        return false;

#if defined(TA_ACCEL_HAS_METAL)
    return TA_accel_metal_sma_double(inReal + trailingIdx,
                                     inputCount,
                                     optInTimePeriod,
                                     outReal,
                                     outputCount);
#else
    (void)inReal;
    (void)optInTimePeriod;
    (void)startIdx;
    (void)endIdx;
    (void)lookbackTotal;
    (void)outReal;
    return false;
#endif
}

bool TA_accel_sma_float(const float *inReal,
                        int optInTimePeriod,
                        int startIdx,
                        int endIdx,
                        int lookbackTotal,
                        double *outReal)
{
    int trailingIdx = 0;
    int inputCount = 0;
    int outputCount = 0;

    if (!inReal || !outReal)
        return false;

    if (!validate_window(optInTimePeriod,
                         startIdx,
                         endIdx,
                         lookbackTotal,
                         &trailingIdx,
                         &inputCount,
                         &outputCount))
        return false;

#if defined(TA_ACCEL_HAS_METAL)
    return TA_accel_metal_sma_float(inReal + trailingIdx,
                                    inputCount,
                                    optInTimePeriod,
                                    outReal,
                                    outputCount);
#else
    (void)inReal;
    (void)optInTimePeriod;
    (void)startIdx;
    (void)endIdx;
    (void)lookbackTotal;
    (void)outReal;
    return false;
#endif
}

bool TA_accel_trima_double(const double *inReal,
                           int optInTimePeriod,
                           int startIdx,
                           int endIdx,
                           int lookbackTotal,
                           double *outReal)
{
    int trailingIdx = 0;
    int inputCount = 0;
    int outputCount = 0;

    if (!inReal || !outReal)
        return false;

    if (!validate_window(optInTimePeriod,
                         startIdx,
                         endIdx,
                         lookbackTotal,
                         &trailingIdx,
                         &inputCount,
                         &outputCount))
        return false;

    if (optInTimePeriod < 2)
        return false;

    int stageOnePeriod;
    int stageTwoPeriod;
    if ((optInTimePeriod % 2) == 0) {
        stageOnePeriod = optInTimePeriod / 2;
        stageTwoPeriod = (optInTimePeriod / 2) + 1;
    } else {
        stageOnePeriod = (optInTimePeriod + 1) / 2;
        stageTwoPeriod = stageOnePeriod;
    }

    if (stageOnePeriod < 2 || stageTwoPeriod < 2)
        return false;

    int stageOneOutCount = inputCount - stageOnePeriod + 1;
    if (stageOneOutCount <= 0)
        return false;

    int stageTwoOutCount = stageOneOutCount - stageTwoPeriod + 1;
    if (stageTwoOutCount != outputCount || stageTwoOutCount <= 0)
        return false;

#if defined(TA_ACCEL_HAS_METAL)
    double *stageOne = (double *)TA_Malloc(sizeof(double) * (size_t)stageOneOutCount);
    if (!stageOne)
        return false;

    bool ok = TA_accel_metal_sma_double(inReal + trailingIdx,
                                        inputCount,
                                        stageOnePeriod,
                                        stageOne,
                                        stageOneOutCount);
    if (ok) {
        ok = TA_accel_metal_sma_double(stageOne,
                                       stageOneOutCount,
                                       stageTwoPeriod,
                                       outReal,
                                       outputCount);
    }

    TA_Free(stageOne);
    return ok;
#else
    (void)inReal;
    (void)optInTimePeriod;
    (void)startIdx;
    (void)endIdx;
    (void)lookbackTotal;
    (void)outReal;
    return false;
#endif
}

bool TA_accel_wma_double(const double *inReal,
                         int optInTimePeriod,
                         int startIdx,
                         int endIdx,
                         int lookbackTotal,
                         double *outReal)
{
    int trailingIdx = 0;
    int inputCount = 0;
    int outputCount = 0;

    if (!inReal || !outReal)
        return false;

    if (!validate_window(optInTimePeriod,
                         startIdx,
                         endIdx,
                         lookbackTotal,
                         &trailingIdx,
                         &inputCount,
                         &outputCount))
        return false;

#if defined(TA_ACCEL_HAS_METAL)
    return TA_accel_metal_wma_double(inReal + trailingIdx,
                                     inputCount,
                                     optInTimePeriod,
                                     outReal,
                                     outputCount);
#else
    (void)inReal;
    (void)optInTimePeriod;
    (void)startIdx;
    (void)endIdx;
    (void)lookbackTotal;
    (void)outReal;
    return false;
#endif
}
