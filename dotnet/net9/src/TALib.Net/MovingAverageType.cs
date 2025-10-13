namespace TALib.Net;

/// <summary>
/// Moving average algorithm identifiers used by TA-Lib (TA_MAType).
/// </summary>
public enum MovingAverageType
{
    /// <summary>Simple moving average.</summary>
    Sma = 0,

    /// <summary>Exponential moving average.</summary>
    Ema = 1,

    /// <summary>Weighted moving average.</summary>
    Wma = 2,

    /// <summary>Double exponential moving average.</summary>
    Dema = 3,

    /// <summary>Triple exponential moving average.</summary>
    Tema = 4,

    /// <summary>Triangular moving average.</summary>
    Trima = 5,

    /// <summary>Kaufman's adaptive moving average.</summary>
    Kama = 6,

    /// <summary>MESA adaptive moving average.</summary>
    Mama = 7,

    /// <summary>Triple exponential moving average (T3).</summary>
    T3 = 8
}
