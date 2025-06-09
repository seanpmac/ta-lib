/* NPP accelerated implementation for AVGPRICE */
#include "ta_func.h"
#include "ta_memory.h"

#include <npps.h>

TA_RetCode TA_AVGPRICE_NPP( int startIdx,
                            int endIdx,
                            const double inOpen[],
                            const double inHigh[],
                            const double inLow[],
                            const double inClose[],
                            int *outBegIdx,
                            int *outNBElement,
                            double outReal[] )
{
    if( startIdx < 0 )
        return TA_OUT_OF_RANGE_START_INDEX;
    if( endIdx < startIdx )
        return TA_OUT_OF_RANGE_END_INDEX;

    int length = endIdx - startIdx + 1;
    NppStatus s;
    /* outReal = inOpen + inHigh */
    s = nppsAdd_64f((Npp64f*)&inOpen[startIdx], (Npp64f*)&inHigh[startIdx], (Npp64f*)outReal, length);
    if( s != NPP_SUCCESS )
        return TA_INTERNAL_ERROR(0);
    /* outReal += inLow */
    s = nppsAdd_64f_I((Npp64f*)&inLow[startIdx], (Npp64f*)outReal, length);
    if( s != NPP_SUCCESS )
        return TA_INTERNAL_ERROR(0);
    /* outReal += inClose */
    s = nppsAdd_64f_I((Npp64f*)&inClose[startIdx], (Npp64f*)outReal, length);
    if( s != NPP_SUCCESS )
        return TA_INTERNAL_ERROR(0);
    /* outReal /= 4 */
    s = nppsMulC_64f_I(0.25, (Npp64f*)outReal, length);
    if( s != NPP_SUCCESS )
        return TA_INTERNAL_ERROR(0);

    *outBegIdx = startIdx;
    *outNBElement = length;
    return TA_SUCCESS;
}

TA_RetCode TA_S_AVGPRICE_NPP( int startIdx,
                              int endIdx,
                              const float inOpen[],
                              const float inHigh[],
                              const float inLow[],
                              const float inClose[],
                              int *outBegIdx,
                              int *outNBElement,
                              double outReal[] )
{
    if( startIdx < 0 )
        return TA_OUT_OF_RANGE_START_INDEX;
    if( endIdx < startIdx )
        return TA_OUT_OF_RANGE_END_INDEX;

    int length = endIdx - startIdx + 1;
    NppStatus s;
    s = nppsAdd_32f((Npp32f*)&inOpen[startIdx], (Npp32f*)&inHigh[startIdx], (Npp32f*)outReal, length);
    if( s != NPP_SUCCESS )
        return TA_INTERNAL_ERROR(0);
    s = nppsAdd_32f_I((Npp32f*)&inLow[startIdx], (Npp32f*)outReal, length);
    if( s != NPP_SUCCESS )
        return TA_INTERNAL_ERROR(0);
    s = nppsAdd_32f_I((Npp32f*)&inClose[startIdx], (Npp32f*)outReal, length);
    if( s != NPP_SUCCESS )
        return TA_INTERNAL_ERROR(0);
    s = nppsMulC_32f_I(0.25f, (Npp32f*)outReal, length);
    if( s != NPP_SUCCESS )
        return TA_INTERNAL_ERROR(0);

    *outBegIdx = startIdx;
    *outNBElement = length;
    return TA_SUCCESS;
}
