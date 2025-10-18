/* TA-LIB Copyright (c) 1999-2025, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TA_VARIANCE_H
#define TA_VARIANCE_H

#include <math.h>

/*
 * Welford's Online Variance Algorithm
 * ===================================
 * 
 * This header provides numerically stable variance and standard deviation
 * calculations using Welford's online algorithm. Unlike the computational
 * formula Var(X) = E[X²] - E[X]², Welford's method avoids catastrophic
 * cancellation when dealing with large values or long periods.
 * 
 * Key Advantages:
 * - Numerically stable: no catastrophic cancellation
 * - Single-pass: accumulates statistics incrementally
 * - Memory efficient: constant O(1) storage regardless of period
 * - Incremental updates: can add/remove values from rolling windows
 * 
 * Algorithm:
 * For each new value x:
 *   count = count + 1
 *   delta = x - mean
 *   mean = mean + delta / count
 *   delta2 = x - mean
 *   M2 = M2 + delta * delta2
 *   variance = M2 / count (population) or M2 / (count - 1) (sample)
 * 
 * For rolling windows (removing old values):
 *   count = count - 1
 *   delta = x_old - mean
 *   mean = mean - delta / count
 *   delta2 = x_old - mean  
 *   M2 = M2 - delta * delta2
 * 
 * References:
 * - Welford, B. P. (1962). "Note on a method for calculating corrected
 *   sums of squares and products". Technometrics 4(3):419-420.
 * - Knuth, Donald E. (1998). The Art of Computer Programming, volume 2:
 *   Seminumerical Algorithms, 3rd ed., Addison-Wesley.
 */

/**
 * Welford accumulator for online variance calculation
 */
typedef struct {
    double mean;     /* Current mean value */
    double M2;       /* Sum of squared differences from mean */
    int count;       /* Number of values accumulated */
} TA_WelfordAccumulator;

/**
 * Initialize a Welford accumulator
 */
static inline void TA_Welford_Init(TA_WelfordAccumulator *acc) {
    acc->mean = 0.0;
    acc->M2 = 0.0;
    acc->count = 0;
}

/**
 * Add a value to the Welford accumulator
 * 
 * @param acc Accumulator to update
 * @param value New value to add
 */
static inline void TA_Welford_Add(TA_WelfordAccumulator *acc, double value) {
    acc->count++;
    double delta = value - acc->mean;
    acc->mean += delta / acc->count;
    double delta2 = value - acc->mean;
    acc->M2 += delta * delta2;
}

/**
 * Remove a value from the Welford accumulator (for rolling windows)
 * 
 * @param acc Accumulator to update
 * @param value Old value to remove
 */
static inline void TA_Welford_Remove(TA_WelfordAccumulator *acc, double value) {
    if (acc->count <= 0) return;
    
    if (acc->count == 1) {
        /* Last element - reset to avoid accumulated floating point errors */
        acc->mean = 0.0;
        acc->M2 = 0.0;
        acc->count = 0;
        return;
    }
    
    double delta = value - acc->mean;
    acc->count--;
    acc->mean -= delta / acc->count;
    double delta2 = value - acc->mean;
    acc->M2 -= delta * delta2;
    
    /* Guard against floating point errors making M2 slightly negative */
    if (acc->M2 < 0.0) acc->M2 = 0.0;
}

/**
 * Get current population variance
 * 
 * @param acc Accumulator
 * @return Population variance (divide by n)
 */
static inline double TA_Welford_GetVariance(const TA_WelfordAccumulator *acc) {
    if (acc->count <= 0) return 0.0;
    return acc->M2 / acc->count;
}

/**
 * Get current sample variance
 * 
 * @param acc Accumulator
 * @return Sample variance (divide by n-1)
 */
static inline double TA_Welford_GetSampleVariance(const TA_WelfordAccumulator *acc) {
    if (acc->count <= 1) return 0.0;
    return acc->M2 / (acc->count - 1);
}

/**
 * Get current mean
 * 
 * @param acc Accumulator
 * @return Current mean value
 */
static inline double TA_Welford_GetMean(const TA_WelfordAccumulator *acc) {
    return acc->mean;
}

/**
 * Compute rolling variance over an array using Welford's algorithm
 * This is a drop-in replacement for the computational formula approach.
 * 
 * @param startIdx Starting index in input array
 * @param endIdx Ending index in input array
 * @param inReal Input price array
 * @param period Rolling window period
 * @param outReal Output variance array (must be allocated)
 */
static inline void TA_Welford_RollingVariance(
    int startIdx,
    int endIdx,
    const double *inReal,
    int period,
    double *outReal)
{
    TA_WelfordAccumulator acc;
    TA_Welford_Init(&acc);
    
    int trailingIdx = startIdx - (period - 1);
    int i;
    
    /* Initialize accumulator with first window (excluding startIdx value) */
    for (i = trailingIdx; i < startIdx; i++) {
        TA_Welford_Add(&acc, inReal[i]);
    }
    
    /* Process remaining values with rolling window */
    int outIdx = 0;
    for (i = startIdx; i <= endIdx; i++) {
        /* Add new value to window */
        TA_Welford_Add(&acc, inReal[i]);
        
        /* Output variance for current window */
        outReal[outIdx++] = TA_Welford_GetVariance(&acc);
        
        /* Remove oldest value from window for next iteration */
        TA_Welford_Remove(&acc, inReal[trailingIdx++]);
    }
}

/**
 * Float precision version of rolling variance
 */
static inline void TA_Welford_RollingVariance_F(
    int startIdx,
    int endIdx,
    const float *inReal,
    int period,
    double *outReal)
{
    TA_WelfordAccumulator acc;
    TA_Welford_Init(&acc);
    
    int trailingIdx = startIdx - (period - 1);
    int i;
    
    /* Initialize accumulator with first window (excluding startIdx value) */
    for (i = trailingIdx; i < startIdx; i++) {
        TA_Welford_Add(&acc, (double)inReal[i]);
    }
    
    /* Process remaining values with rolling window */
    int outIdx = 0;
    for (i = startIdx; i <= endIdx; i++) {
        /* Add new value to window */
        TA_Welford_Add(&acc, (double)inReal[i]);
        
        /* Output variance for current window */
        outReal[outIdx++] = TA_Welford_GetVariance(&acc);
        
        /* Remove oldest value from window for next iteration */
        TA_Welford_Remove(&acc, (double)inReal[trailingIdx++]);
    }
}

/**
 * Compute average deviation (mean absolute deviation) using Welford mean
 * This provides a stable mean calculation for the two-pass AVGDEV algorithm.
 * 
 * @param startIdx Starting index in input array
 * @param endIdx Ending index in input array
 * @param inReal Input price array
 * @param period Rolling window period
 * @param outReal Output average deviation array (must be allocated)
 */
static inline void TA_Welford_RollingAvgDev(
    int startIdx,
    int endIdx,
    const double *inReal,
    int period,
    double *outReal)
{
    TA_WelfordAccumulator meanAcc;
    int trailingIdx = startIdx - (period - 1);
    int i, outIdx = 0;
    
    for (int pos = startIdx; pos <= endIdx; pos++) {
        /* Compute mean using Welford for current window */
        TA_Welford_Init(&meanAcc);
        for (i = pos - (period - 1); i <= pos; i++) {
            TA_Welford_Add(&meanAcc, inReal[i]);
        }
        double mean = TA_Welford_GetMean(&meanAcc);
        
        /* Compute mean absolute deviation */
        double sumDev = 0.0;
        for (i = pos - (period - 1); i <= pos; i++) {
            sumDev += fabs(inReal[i] - mean);
        }
        outReal[outIdx++] = sumDev / period;
    }
}

/**
 * Float precision version of rolling average deviation
 */
static inline void TA_Welford_RollingAvgDev_F(
    int startIdx,
    int endIdx,
    const float *inReal,
    int period,
    double *outReal)
{
    TA_WelfordAccumulator meanAcc;
    int trailingIdx = startIdx - (period - 1);
    int i, outIdx = 0;
    
    for (int pos = startIdx; pos <= endIdx; pos++) {
        /* Compute mean using Welford for current window */
        TA_Welford_Init(&meanAcc);
        for (i = pos - (period - 1); i <= pos; i++) {
            TA_Welford_Add(&meanAcc, (double)inReal[i]);
        }
        double mean = TA_Welford_GetMean(&meanAcc);
        
        /* Compute mean absolute deviation */
        double sumDev = 0.0;
        for (i = pos - (period - 1); i <= pos; i++) {
            sumDev += fabs((double)inReal[i] - mean);
        }
        outReal[outIdx++] = sumDev / period;
    }
}

#endif /* TA_VARIANCE_H */
