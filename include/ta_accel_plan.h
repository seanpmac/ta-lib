#ifndef TA_ACCEL_PLAN_H
#define TA_ACCEL_PLAN_H

#include "ta_common.h"

typedef enum
{
    TA_ACCEL_INDICATOR_SMA = 0,
    TA_ACCEL_INDICATOR_EMA = 1,
    TA_ACCEL_INDICATOR_RSI = 2,
    TA_ACCEL_INDICATOR_TRIMA = 3,
    TA_ACCEL_INDICATOR_WMA = 4
} TA_AccelIndicatorType;

typedef struct
{
    unsigned int period;  /* Indicator period */
    double *outReal;      /* Caller-allocated output buffer */
    int outBegIdx;
    int outNBElement;
} TA_AccelIndicatorOutput;

typedef struct
{
    TA_AccelIndicatorType type;
    unsigned int timeframeSeconds;
    const double *input;
    int length;
    TA_AccelIndicatorOutput *outputs;
    unsigned int outputCount;
} TA_AccelIndicatorRequest;

TA_RetCode TA_accel_execute_plan(const TA_AccelIndicatorRequest *requests,
                                 unsigned int requestCount);

#endif /* TA_ACCEL_PLAN_H */
