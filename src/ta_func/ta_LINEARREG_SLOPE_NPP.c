/* NPP accelerated implementation for LINEARREG_SLOPE */
#include "ta_func.h"
#include "ta_memory.h"
#include <npps.h>

TA_RetCode TA_LINEARREG_SLOPE_NPP( int startIdx,
                                   int endIdx,
                                   const double inReal[],
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
        optInTimePeriod = 14;
    else if( optInTimePeriod < 2 || optInTimePeriod > 100000 )
        return TA_BAD_PARAM;

    int lookbackTotal = optInTimePeriod - 1;
    if( startIdx < lookbackTotal )
        startIdx = lookbackTotal;
    if( startIdx > endIdx ) {
        *outBegIdx = 0;
        *outNBElement = 0;
        return TA_SUCCESS;
    }

    double *weights = TA_Malloc(sizeof(double)*optInTimePeriod);
    if(!weights) return TA_ALLOC_ERR;
    for(int i=0;i<optInTimePeriod;i++)
        weights[i] = i;

    double SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
    double SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6.0;
    double Divisor = SumX * SumX - optInTimePeriod * SumXSqr;

    int length = endIdx - startIdx + 1;
    for( int idx = 0; idx < length; ++idx ) {
        const double *data = &inReal[idx + startIdx - lookbackTotal];
        Npp64f sumY, sumXY;
        NppStatus s = nppsSum_64f((Npp64f*)data, optInTimePeriod, &sumY);
        if( s != NPP_SUCCESS ){ TA_Free(weights); return TA_INTERNAL_ERROR(0); }
        s = nppsDotProd_64f((Npp64f*)weights, (Npp64f*)data, optInTimePeriod, &sumXY);
        if( s != NPP_SUCCESS ){ TA_Free(weights); return TA_INTERNAL_ERROR(0); }
        outReal[idx] = ( optInTimePeriod * sumXY - SumX * sumY) / Divisor;
    }

    TA_Free(weights);
    *outBegIdx = startIdx;
    *outNBElement = length;
    return TA_SUCCESS;
}

TA_RetCode TA_S_LINEARREG_SLOPE_NPP( int startIdx,
                                     int endIdx,
                                     const float inReal[],
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
        optInTimePeriod = 14;
    else if( optInTimePeriod < 2 || optInTimePeriod > 100000 )
        return TA_BAD_PARAM;

    int lookbackTotal = optInTimePeriod - 1;
    if( startIdx < lookbackTotal )
        startIdx = lookbackTotal;
    if( startIdx > endIdx ) {
        *outBegIdx = 0;
        *outNBElement = 0;
        return TA_SUCCESS;
    }

    float *weights = TA_Malloc(sizeof(float)*optInTimePeriod);
    if(!weights) return TA_ALLOC_ERR;
    for(int i=0;i<optInTimePeriod;i++)
        weights[i] = (float)i;

    double SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
    double SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6.0;
    double Divisor = SumX * SumX - optInTimePeriod * SumXSqr;

    int length = endIdx - startIdx + 1;
    for( int idx = 0; idx < length; ++idx ) {
        const float *data = &inReal[idx + startIdx - lookbackTotal];
        Npp32f sumY, sumXY;
        NppStatus s = nppsSum_32f((Npp32f*)data, optInTimePeriod, &sumY);
        if( s != NPP_SUCCESS ){ TA_Free(weights); return TA_INTERNAL_ERROR(0); }
        s = nppsDotProd_32f((Npp32f*)weights, (Npp32f*)data, optInTimePeriod, &sumXY);
        if( s != NPP_SUCCESS ){ TA_Free(weights); return TA_INTERNAL_ERROR(0); }
        outReal[idx] = ( optInTimePeriod * sumXY - SumX * sumY) / Divisor;
    }

    TA_Free(weights);
    *outBegIdx = startIdx;
    *outNBElement = length;
    return TA_SUCCESS;
}
