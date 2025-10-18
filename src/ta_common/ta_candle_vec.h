/* Vectorized helpers for candlestick range computations */
#ifndef TA_CANDLE_VEC_H
#define TA_CANDLE_VEC_H

#include <stddef.h>
#include <math.h>

#include "ta_global.h"

#if defined(_OPENMP)
  #include <omp.h>
#endif

#ifndef TA_RESTRICT
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#    define TA_RESTRICT restrict
#  elif defined(_MSC_VER)
#    define TA_RESTRICT __restrict
#  else
#    define TA_RESTRICT
#  endif
#endif

static inline double TA_CandleRealBodyScalar(double open, double close)
{
    return fabs(close - open);
}

static inline double TA_CandleUpperShadowScalar(double open, double high, double close)
{
    const double top = (close >= open) ? close : open;
    return high - top;
}

static inline double TA_CandleLowerShadowScalar(double open, double low, double close)
{
    const double bottom = (close >= open) ? open : close;
    return bottom - low;
}

static inline double TA_CandleHighLowRangeScalar(double high, double low)
{
    return high - low;
}

static inline void TA_CandleRealBodyVector(const double *open,
                                           const double *close,
                                           int start,
                                           int length,
                                           double *dest)
{
    if( length <= 0 || start < 0 || !open || !close || !dest )
        return;

    const double *openPtr = open + start;
    const double *closePtr = close + start;
    int i;
#if defined(_OPENMP)
    #pragma omp simd
#endif
    for( i = 0; i < length; ++i )
    {
        dest[i] = TA_CandleRealBodyScalar(openPtr[i], closePtr[i]);
    }
}

static inline void TA_CandleRangeVector(const TA_CandleSetting *setting,
                                        const double *open,
                                        const double *high,
                                        const double *low,
                                        const double *close,
                                        int start,
                                        int length,
                                        double *dest)
{
    if( !setting || length <= 0 || start < 0 || !open || !high || !low || !close || !dest )
        return;

    const double *openPtr = open + start;
    const double *highPtr = high + start;
    const double *lowPtr = low + start;
    const double *closePtr = close + start;
    int i;

    switch( setting->rangeType )
    {
    case TA_RangeType_RealBody:
#if defined(_OPENMP)
        #pragma omp simd
#endif
        for( i = 0; i < length; ++i )
        {
            dest[i] = TA_CandleRealBodyScalar(openPtr[i], closePtr[i]);
        }
        break;
    case TA_RangeType_HighLow:
#if defined(_OPENMP)
        #pragma omp simd
#endif
        for( i = 0; i < length; ++i )
        {
            dest[i] = TA_CandleHighLowRangeScalar(highPtr[i], lowPtr[i]);
        }
        break;
    case TA_RangeType_Shadows:
#if defined(_OPENMP)
        #pragma omp simd
#endif
        for( i = 0; i < length; ++i )
        {
            const double openVal = openPtr[i];
            const double closeVal = closePtr[i];
            dest[i] = TA_CandleUpperShadowScalar(openVal, highPtr[i], closeVal)
                    + TA_CandleLowerShadowScalar(openVal, lowPtr[i], closeVal);
        }
        break;
    default:
        for( i = 0; i < length; ++i )
        {
            dest[i] = 0.0;
        }
        break;
    }
}

static inline double TA_CandleAverageFromTotal(const TA_CandleSetting *setting,
                                               double periodTotal,
                                               double currentRange)
{
    if( !setting )
        return 0.0;

    const double divisor = (setting->rangeType == TA_RangeType_Shadows) ? 2.0 : 1.0;

    if( setting->avgPeriod != 0 )
        return setting->factor * (periodTotal / (double)setting->avgPeriod) / divisor;

    return setting->factor * (currentRange / divisor);
}

#endif /* TA_CANDLE_VEC_H */
