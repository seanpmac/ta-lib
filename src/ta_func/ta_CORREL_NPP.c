/* NPP accelerated implementation for CORREL */
#include "ta_func.h"
#include "ta_memory.h"
#include <npps.h>
#include <math.h>

TA_RetCode TA_CORREL_NPP( int startIdx,
                          int endIdx,
                          const double inReal0[],
                          const double inReal1[],
                          int optInTimePeriod,
                          int *outBegIdx,
                          int *outNBElement,
                          double outReal[] )
{
    if( startIdx < 0 )
        return TA_OUT_OF_RANGE_START_INDEX;
    if( endIdx < startIdx )
        return TA_OUT_OF_RANGE_END_INDEX;

    if( optInTimePeriod == TA_INTEGER_DEFAULT )
        optInTimePeriod = 30;
    else if( optInTimePeriod < 1 || optInTimePeriod > 100000 )
        return TA_BAD_PARAM;

    int lookbackTotal = optInTimePeriod - 1;
    if( startIdx < lookbackTotal )
        startIdx = lookbackTotal;
    if( startIdx > endIdx ) {
        *outBegIdx = 0;
        *outNBElement = 0;
        return TA_SUCCESS;
    }

    int length = endIdx - startIdx + 1;
    for( int idx = 0; idx < length; ++idx ) {
        const double *xPtr = &inReal0[idx + startIdx - lookbackTotal];
        const double *yPtr = &inReal1[idx + startIdx - lookbackTotal];
        Npp64f sumX, sumY, sumXY, sumX2, sumY2;
        NppStatus s;
        s = nppsSum_64f((Npp64f*)xPtr, optInTimePeriod, &sumX);
        if( s != NPP_SUCCESS ) return TA_INTERNAL_ERROR(0);
        s = nppsSum_64f((Npp64f*)yPtr, optInTimePeriod, &sumY);
        if( s != NPP_SUCCESS ) return TA_INTERNAL_ERROR(0);
        s = nppsDotProd_64f((Npp64f*)xPtr, (Npp64f*)yPtr, optInTimePeriod, &sumXY);
        if( s != NPP_SUCCESS ) return TA_INTERNAL_ERROR(0);
        s = nppsDotProd_64f((Npp64f*)xPtr, (Npp64f*)xPtr, optInTimePeriod, &sumX2);
        if( s != NPP_SUCCESS ) return TA_INTERNAL_ERROR(0);
        s = nppsDotProd_64f((Npp64f*)yPtr, (Npp64f*)yPtr, optInTimePeriod, &sumY2);
        if( s != NPP_SUCCESS ) return TA_INTERNAL_ERROR(0);
        double numerator = sumXY - (sumX * sumY) / optInTimePeriod;
        double denom = (sumX2 - (sumX * sumX) / optInTimePeriod) *
                       (sumY2 - (sumY * sumY) / optInTimePeriod);
        if( denom <= 0.0 )
            outReal[idx] = 0.0;
        else
            outReal[idx] = numerator / sqrt(denom);
    }
    *outBegIdx = startIdx;
    *outNBElement = length;
    return TA_SUCCESS;
}

TA_RetCode TA_S_CORREL_NPP( int startIdx,
                            int endIdx,
                            const float inReal0[],
                            const float inReal1[],
                            int optInTimePeriod,
                            int *outBegIdx,
                            int *outNBElement,
                            double outReal[] )
{
    if( startIdx < 0 )
        return TA_OUT_OF_RANGE_START_INDEX;
    if( endIdx < startIdx )
        return TA_OUT_OF_RANGE_END_INDEX;

    if( optInTimePeriod == TA_INTEGER_DEFAULT )
        optInTimePeriod = 30;
    else if( optInTimePeriod < 1 || optInTimePeriod > 100000 )
        return TA_BAD_PARAM;

    int lookbackTotal = optInTimePeriod - 1;
    if( startIdx < lookbackTotal )
        startIdx = lookbackTotal;
    if( startIdx > endIdx ) {
        *outBegIdx = 0;
        *outNBElement = 0;
        return TA_SUCCESS;
    }

    int length = endIdx - startIdx + 1;
    for( int idx = 0; idx < length; ++idx ) {
        const float *xPtr = &inReal0[idx + startIdx - lookbackTotal];
        const float *yPtr = &inReal1[idx + startIdx - lookbackTotal];
        Npp32f sumX, sumY, sumXY, sumX2, sumY2;
        NppStatus s;
        s = nppsSum_32f((Npp32f*)xPtr, optInTimePeriod, &sumX);
        if( s != NPP_SUCCESS ) return TA_INTERNAL_ERROR(0);
        s = nppsSum_32f((Npp32f*)yPtr, optInTimePeriod, &sumY);
        if( s != NPP_SUCCESS ) return TA_INTERNAL_ERROR(0);
        s = nppsDotProd_32f((Npp32f*)xPtr, (Npp32f*)yPtr, optInTimePeriod, &sumXY);
        if( s != NPP_SUCCESS ) return TA_INTERNAL_ERROR(0);
        s = nppsDotProd_32f((Npp32f*)xPtr, (Npp32f*)xPtr, optInTimePeriod, &sumX2);
        if( s != NPP_SUCCESS ) return TA_INTERNAL_ERROR(0);
        s = nppsDotProd_32f((Npp32f*)yPtr, (Npp32f*)yPtr, optInTimePeriod, &sumY2);
        if( s != NPP_SUCCESS ) return TA_INTERNAL_ERROR(0);
        double numerator = sumXY - (sumX * sumY) / optInTimePeriod;
        double denom = (sumX2 - (sumX * sumX) / optInTimePeriod) *
                       (sumY2 - (sumY * sumY) / optInTimePeriod);
        if( denom <= 0.0 )
            outReal[idx] = 0.0;
        else
            outReal[idx] = numerator / sqrt(denom);
    }
    *outBegIdx = startIdx;
    *outNBElement = length;
    return TA_SUCCESS;
}
