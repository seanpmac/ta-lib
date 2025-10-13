namespace TALib.Net;

/// <summary>
/// Return codes from the native TA-Lib C library (TA_RetCode).
/// </summary>
public enum TaRetCode
{
    /// <summary>Operation completed successfully.</summary>
    Success = 0,

    /// <summary>TA_Initialize was not successfully called.</summary>
    LibNotInitialize = 1,

    /// <summary>A parameter value falls outside the allowed range.</summary>
    BadParam = 2,

    /// <summary>Native allocation failed, usually due to low memory.</summary>
    AllocErr = 3,

    /// <summary>The requested group identifier does not exist.</summary>
    GroupNotFound = 4,

    /// <summary>The requested function identifier does not exist.</summary>
    FuncNotFound = 5,

    /// <summary>An invalid handle was supplied.</summary>
    InvalidHandle = 6,

    /// <summary>An invalid parameter holder pointer was supplied.</summary>
    InvalidParamHolder = 7,

    /// <summary>The parameter holder type is not valid for this function.</summary>
    InvalidParamHolderType = 8,

    /// <summary>The parameter holder function pointer is invalid.</summary>
    InvalidParamFunction = 9,

    /// <summary>Not all required inputs were initialized.</summary>
    InputNotAllInitialize = 10,

    /// <summary>Not all required outputs were initialized.</summary>
    OutputNotAllInitialize = 11,

    /// <summary>The start index parameter is outside the valid range.</summary>
    OutOfRangeStartIndex = 12,

    /// <summary>The end index parameter is outside the valid range.</summary>
    OutOfRangeEndIndex = 13,

    /// <summary>The requested list type is not valid.</summary>
    InvalidListType = 14,

    /// <summary>The supplied object is not valid.</summary>
    BadObject = 15,

    /// <summary>The requested feature is not supported.</summary>
    NotSupported = 16,

    /// <summary>An unexpected internal error occurred.</summary>
    InternalError = 5000,

    /// <summary>An unknown error occurred.</summary>
    UnknownErr = 0xFFFF
}
