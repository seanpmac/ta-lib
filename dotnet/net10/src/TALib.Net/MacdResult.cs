namespace TALib.Net;

/// <summary>
/// Holds the output series produced by the MACD indicator.
/// </summary>
public sealed class MacdResult
{
    internal MacdResult(double[] macd, double[] signal, double[] histogram)
    {
        Macd = macd;
        Signal = signal;
        Histogram = histogram;
    }

    /// <summary>MACD line values.</summary>
    public double[] Macd { get; }

    /// <summary>Signal line values.</summary>
    public double[] Signal { get; }

    /// <summary>Histogram values.</summary>
    public double[] Histogram { get; }
}
