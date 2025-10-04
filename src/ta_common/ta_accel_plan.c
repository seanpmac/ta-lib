#include "ta_accel_plan.h"
#include "ta_accel.h"
#include "ta_memory.h"

#include <math.h>

static TA_RetCode compute_sma_one(const double *input,
                                  int length,
                                  unsigned int period,
                                  TA_AccelIndicatorOutput *out)
{
    if (!input || !out)
        return TA_BAD_PARAM;
    if (period < 2)
        return TA_BAD_PARAM;
    int lookback = (int)period - 1;
    if (length <= lookback) {
        out->outBegIdx = 0;
        out->outNBElement = 0;
        return TA_SUCCESS;
    }

    int startIdx = lookback;
    int endIdx = length - 1;
    int expectedNbElement = endIdx - startIdx + 1;
    if (!out->outReal)
        return TA_BAD_PARAM;

    if (TA_accel_is_ready()) {
        if (TA_accel_sma_double(input,
                                (int)period,
                                startIdx,
                                endIdx,
                                lookback,
                                out->outReal)) {
            out->outBegIdx = startIdx;
            out->outNBElement = expectedNbElement;
            return TA_SUCCESS;
        }
    }

    double periodTotal = 0.0;
    int trailingIdx = startIdx - lookback;
    int i = trailingIdx;
    if ((int)period > 1) {
        while (i < startIdx)
            periodTotal += input[i++];
    }

    int outIdx = 0;
    do {
        periodTotal += input[i++];
        double tempReal = periodTotal;
        periodTotal -= input[trailingIdx++];
        out->outReal[outIdx++] = tempReal / (double)period;
    } while (i <= endIdx);

    out->outBegIdx = startIdx;
    out->outNBElement = outIdx;
    return TA_SUCCESS;
}

static TA_RetCode compute_ema_one(const double *input,
                                  int length,
                                  unsigned int period,
                                  TA_AccelIndicatorOutput *out)
{
    if (!input || !out)
        return TA_BAD_PARAM;
    if (period < 2)
        return TA_BAD_PARAM;
    int lookback = (int)period - 1;
    if (length <= lookback) {
        out->outBegIdx = 0;
        out->outNBElement = 0;
        return TA_SUCCESS;
    }

    if (!out->outReal)
        return TA_BAD_PARAM;

    double multiplier = 2.0 / ((double)period + 1.0);
    double sum = 0.0;
    for (unsigned int i = 0; i < period; ++i)
        sum += input[i];
    double prevEMA = sum / (double)period;
    int outIdx = 0;
    out->outReal[outIdx++] = prevEMA;

    for (int i = (int)period; i < length; ++i) {
        double price = input[i];
        prevEMA = ((price - prevEMA) * multiplier) + prevEMA;
        out->outReal[outIdx++] = prevEMA;
    }

    out->outBegIdx = lookback;
    out->outNBElement = outIdx;
    return TA_SUCCESS;
}

static TA_RetCode compute_rsi_one(const double *input,
                                  int length,
                                  unsigned int period,
                                  TA_AccelIndicatorOutput *out)
{
    if (!input || !out)
        return TA_BAD_PARAM;
    if (period < 2)
        return TA_BAD_PARAM;
    if (!out->outReal)
        return TA_BAD_PARAM;

    if (length <= (int)period) {
        out->outBegIdx = 0;
        out->outNBElement = 0;
        return TA_SUCCESS;
    }

    double gain = 0.0;
    double loss = 0.0;
    for (unsigned int i = 1; i <= period; ++i) {
        double diff = input[i] - input[i - 1];
        if (diff >= 0.0)
            gain += diff;
        else
            loss -= diff;
    }
    double avgGain = gain / (double)period;
    double avgLoss = loss / (double)period;

    int outIdx = 0;
    int startIdx = (int)period;
    int endIdx = length - 1;

    double rs;
    double rsi;
    if (avgLoss == 0.0)
        rsi = 100.0;
    else if (avgGain == 0.0)
        rsi = 0.0;
    else {
        rs = avgGain / avgLoss;
        rsi = 100.0 - (100.0 / (1.0 + rs));
    }
    out->outReal[outIdx++] = rsi;

    for (int i = startIdx + 1; i <= endIdx; ++i) {
        double diff = input[i] - input[i - 1];
        double addGain = diff > 0.0 ? diff : 0.0;
        double addLoss = diff < 0.0 ? -diff : 0.0;
        avgGain = ((avgGain * ((double)period - 1.0)) + addGain) / (double)period;
        avgLoss = ((avgLoss * ((double)period - 1.0)) + addLoss) / (double)period;

        if (avgLoss == 0.0)
            rsi = 100.0;
        else if (avgGain == 0.0)
            rsi = 0.0;
        else {
            rs = avgGain / avgLoss;
            rsi = 100.0 - (100.0 / (1.0 + rs));
        }
        out->outReal[outIdx++] = rsi;
    }

    out->outBegIdx = startIdx;
    out->outNBElement = outIdx;
    return TA_SUCCESS;
}

static TA_RetCode compute_trima_one(const double *input,
                                    int length,
                                    unsigned int period,
                                    TA_AccelIndicatorOutput *out)
{
    if (!input || !out)
        return TA_BAD_PARAM;
    if (period < 2)
        return TA_BAD_PARAM;
    int lookback = (int)period - 1;
    if (length <= lookback) {
        out->outBegIdx = 0;
        out->outNBElement = 0;
        return TA_SUCCESS;
    }

    if (!out->outReal)
        return TA_BAD_PARAM;

    int startIdx = lookback;
    int endIdx = length - 1;
    int expectedNbElement = endIdx - startIdx + 1;

    if (TA_accel_is_ready()) {
        if (TA_accel_trima_double(input,
                                  (int)period,
                                  startIdx,
                                  endIdx,
                                  lookback,
                                  out->outReal)) {
            out->outBegIdx = startIdx;
            out->outNBElement = expectedNbElement;
            return TA_SUCCESS;
        }
    }

    double *weights = (double *)TA_Malloc(sizeof(double) * period);
    if (!weights)
        return TA_ALLOC_ERR;

    unsigned int half = period / 2;
    if (period % 2 == 0) {
        for (unsigned int i = 0; i < half; ++i)
            weights[i] = (double)(i + 1);
        for (unsigned int i = half; i < period; ++i)
            weights[i] = (double)(period - i);
    } else {
        for (unsigned int i = 0; i <= half; ++i)
            weights[i] = (double)(i + 1);
        for (unsigned int i = half + 1; i < period; ++i)
            weights[i] = (double)(period - i);
    }

    double totalWeight = 0.0;
    for (unsigned int i = 0; i < period; ++i)
        totalWeight += weights[i];

    int outIdx = 0;

    for (int pos = startIdx; pos <= endIdx; ++pos) {
        double sum = 0.0;
        int base = pos - (int)period + 1;
        for (unsigned int w = 0; w < period; ++w)
            sum += input[base + (int)w] * weights[w];
        out->outReal[outIdx++] = sum / totalWeight;
    }

    TA_Free(weights);
    out->outBegIdx = startIdx;
    out->outNBElement = outIdx;
    return TA_SUCCESS;
}

static TA_RetCode compute_wma_one(const double *input,
                                  int length,
                                  unsigned int period,
                                  TA_AccelIndicatorOutput *out)
{
    if (!input || !out)
        return TA_BAD_PARAM;
    if (period < 2)
        return TA_BAD_PARAM;
    int lookback = (int)period - 1;
    if (length <= lookback) {
        out->outBegIdx = 0;
        out->outNBElement = 0;
        return TA_SUCCESS;
    }

    if (!out->outReal)
        return TA_BAD_PARAM;

    double totalWeight = (double)period * ((double)period + 1.0) / 2.0;
    int startIdx = lookback;
    int endIdx = length - 1;
    int expectedNbElement = endIdx - startIdx + 1;

    if (TA_accel_is_ready()) {
        if (TA_accel_wma_double(input,
                                (int)period,
                                startIdx,
                                endIdx,
                                lookback,
                                out->outReal)) {
            out->outBegIdx = startIdx;
            out->outNBElement = expectedNbElement;
            return TA_SUCCESS;
        }
    }

    int outIdx = 0;

    for (int pos = startIdx; pos <= endIdx; ++pos) {
        double sum = 0.0;
        int base = pos - (int)period + 1;
        for (unsigned int w = 0; w < period; ++w)
            sum += input[base + (int)w] * (double)(w + 1U);
        out->outReal[outIdx++] = sum / totalWeight;
    }

    out->outBegIdx = startIdx;
    out->outNBElement = outIdx;
    return TA_SUCCESS;
}

static TA_RetCode dispatch_indicator(const TA_AccelIndicatorRequest *req)
{
    if (!req)
        return TA_BAD_PARAM;
    if (!req->outputs || req->outputCount == 0)
        return TA_BAD_PARAM;

    for (unsigned int j = 0; j < req->outputCount; ++j) {
        TA_AccelIndicatorOutput *out = &req->outputs[j];
        TA_RetCode rc;
        switch (req->type) {
        case TA_ACCEL_INDICATOR_SMA:
            rc = compute_sma_one(req->input, req->length, out->period, out);
            break;
        case TA_ACCEL_INDICATOR_EMA:
            rc = compute_ema_one(req->input, req->length, out->period, out);
            break;
        case TA_ACCEL_INDICATOR_RSI:
            rc = compute_rsi_one(req->input, req->length, out->period, out);
            break;
        case TA_ACCEL_INDICATOR_TRIMA:
            rc = compute_trima_one(req->input, req->length, out->period, out);
            break;
        case TA_ACCEL_INDICATOR_WMA:
            rc = compute_wma_one(req->input, req->length, out->period, out);
            break;
        default:
            rc = TA_BAD_PARAM;
            break;
        }
        if (rc != TA_SUCCESS)
            return rc;
    }
    return TA_SUCCESS;
}

TA_RetCode TA_accel_execute_plan(const TA_AccelIndicatorRequest *requests,
                                 unsigned int requestCount)
{
    if (!requests || requestCount == 0)
        return TA_BAD_PARAM;

    for (unsigned int i = 0; i < requestCount; ++i) {
        const TA_AccelIndicatorRequest *req = &requests[i];
        if (!req->input || req->length <= 0)
            return TA_BAD_PARAM;
        TA_RetCode rc = dispatch_indicator(req);
        if (rc != TA_SUCCESS)
            return rc;
    }

    return TA_SUCCESS;
}
