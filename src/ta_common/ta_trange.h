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

#ifndef TA_TRANGE_H
#define TA_TRANGE_H

#include <math.h>

/**
 * @file ta_trange.h
 * @brief Optimized True Range calculation utilities
 *
 * Provides inline functions for computing True Range, the foundation for
 * Average True Range (ATR) and Normalized ATR (NATR) indicators.
 *
 * True Range is the greatest of:
 *  1. Current High - Current Low (intraday range)
 *  2. |Previous Close - Current High| (gap up)
 *  3. |Previous Close - Current Low| (gap down)
 *
 * This inline implementation enables:
 * - Direct use by ATR/NATR without function call overhead
 * - SIMD vectorization via compiler optimization
 * - Potential future caching layer for multi-indicator batching
 */

/**
 * Calculate True Range for a single bar
 *
 * @param prevClose Yesterday's closing price
 * @param high Today's high price
 * @param low Today's low price
 * @return True Range value (always >= 0)
 *
 * @note This function uses fabs() for portability. For performance-critical
 *       paths, consider replacing with branchless max() when profiling shows benefit.
 */
static inline double TA_TrueRange(double prevClose, double high, double low)
{
    const double intraday = high - low;                      // val1: H - L
    const double gapHigh  = fabs(prevClose - high);         // val2: |C[-1] - H|
    const double gapLow   = fabs(prevClose - low);          // val3: |C[-1] - L|
    
    double greatest = intraday;
    if (gapHigh > greatest)
        greatest = gapHigh;
    if (gapLow > greatest)
        greatest = gapLow;
    
    return greatest;
}

/**
 * Calculate True Range for a single bar (float precision)
 *
 * @param prevClose Yesterday's closing price
 * @param high Today's high price
 * @param low Today's low price
 * @return True Range value (always >= 0)
 */
static inline float TA_TrueRange_F(float prevClose, float high, float low)
{
    const float intraday = high - low;
    const float gapHigh  = fabsf(prevClose - high);
    const float gapLow   = fabsf(prevClose - low);
    
    float greatest = intraday;
    if (gapHigh > greatest)
        greatest = gapHigh;
    if (gapLow > greatest)
        greatest = gapLow;
    
    return greatest;
}

/**
 * Calculate True Range for array of bars (vectorized)
 *
 * @param startIdx Index of first bar to compute (must be >= 1 since we need prevClose)
 * @param endIdx Index of last bar to compute (inclusive)
 * @param high Array of high prices
 * @param low Array of low prices
 * @param close Array of closing prices
 * @param outTRange Output array for True Range values
 *
 * @note Requires startIdx >= 1 because TR needs previous close.
 *       Output array must have space for (endIdx - startIdx + 1) elements.
 *       This function can be SIMD-optimized by compilers with appropriate pragmas.
 */
static inline void TA_TrueRange_Array(int startIdx, int endIdx,
                                     const double *high, const double *low, const double *close,
                                     double *outTRange)
{
    const int count = endIdx - startIdx + 1;
    
#if defined(_OPENMP)
    #pragma omp simd
#endif
    for (int i = 0; i < count; i++)
    {
        const int idx = startIdx + i;
        outTRange[i] = TA_TrueRange(close[idx - 1], high[idx], low[idx]);
    }
}

/**
 * Calculate True Range for array of bars (float precision, vectorized)
 */
static inline void TA_TrueRange_Array_F(int startIdx, int endIdx,
                                       const float *high, const float *low, const float *close,
                                       double *outTRange)
{
    const int count = endIdx - startIdx + 1;
    
#if defined(_OPENMP)
    #pragma omp simd
#endif
    for (int i = 0; i < count; i++)
    {
        const int idx = startIdx + i;
        outTRange[i] = (double)TA_TrueRange_F(close[idx - 1], high[idx], low[idx]);
    }
}

#endif /* TA_TRANGE_H */
