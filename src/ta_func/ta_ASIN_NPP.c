/* NPP accelerated implementation for ASIN */
#include "ta_func.h"
#include "ta_memory.h"

#include <npps.h>

TA_RetCode TA_ASIN_NPP( int startIdx,
                        int endIdx,
                        const double inReal[],
                        int *outBegIdx,
                        int *outNBElement,
                        double outReal[] )
{
    int length;
    if( startIdx < 0 )
        return TA_OUT_OF_RANGE_START_INDEX;
    if( endIdx < startIdx )
        return TA_OUT_OF_RANGE_END_INDEX;

    length = endIdx - startIdx + 1;
    NppStatus s = nppsAsin_64f((Npp64f*)&inReal[startIdx],
                               (Npp64f*)outReal,
                               length);
    if( s != NPP_SUCCESS )
        return TA_INTERNAL_ERROR(0);
    *outBegIdx = startIdx;
    *outNBElement = length;
    return TA_SUCCESS;
}

TA_RetCode TA_S_ASIN_NPP( int startIdx,
                          int endIdx,
                          const float inReal[],
                          int *outBegIdx,
                          int *outNBElement,
                          double outReal[] )
{
    int length;
    if( startIdx < 0 )
        return TA_OUT_OF_RANGE_START_INDEX;
    if( endIdx < startIdx )
        return TA_OUT_OF_RANGE_END_INDEX;

    length = endIdx - startIdx + 1;
    NppStatus s = nppsAsin_32f((Npp32f*)&inReal[startIdx],
                               (Npp32f*)outReal,
                               length);
    if( s != NPP_SUCCESS )
        return TA_INTERNAL_ERROR(0);
    *outBegIdx = startIdx;
    *outNBElement = length;
    return TA_SUCCESS;
}
