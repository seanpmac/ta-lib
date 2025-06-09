/* NPP accelerated implementation for MIN */
#include "ta_func.h"
#include "ta_memory.h"
#include <npps.h>

TA_RetCode TA_MIN_NPP( int startIdx,
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
        optInTimePeriod = 30;
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

    int length = endIdx - startIdx + 1;
    for( int idx = 0; idx < length; ++idx ) {
        Npp64f val;
        NppStatus s = nppsMin_64f((Npp64f*)&inReal[idx + startIdx - lookbackTotal],
                                  optInTimePeriod,
                                  &val);
        if( s != NPP_SUCCESS )
            return TA_INTERNAL_ERROR(0);
        outReal[idx] = val;
    }
    *outBegIdx = startIdx;
    *outNBElement = length;
    return TA_SUCCESS;
}

TA_RetCode TA_S_MIN_NPP( int startIdx,
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
        optInTimePeriod = 30;
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

    int length = endIdx - startIdx + 1;
    for( int idx = 0; idx < length; ++idx ) {
        Npp32f val;
        NppStatus s = nppsMin_32f((Npp32f*)&inReal[idx + startIdx - lookbackTotal],
                                  optInTimePeriod,
                                  &val);
        if( s != NPP_SUCCESS )
            return TA_INTERNAL_ERROR(0);
        outReal[idx] = val;
    }
    *outBegIdx = startIdx;
    *outNBElement = length;
    return TA_SUCCESS;
}
