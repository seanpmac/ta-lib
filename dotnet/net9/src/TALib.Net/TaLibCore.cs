namespace TALib.Net;

/// <summary>
/// High-level helper APIs for invoking TA-Lib functions from managed code.
/// </summary>
public static class TaLibCore
{
    private static readonly object SyncRoot = new();
    private static bool _initialized;

    /// <summary>
    /// Gets a value indicating whether the native library has been initialized.
    /// </summary>
    public static bool IsInitialized
    {
        get
        {
            lock (SyncRoot)
            {
                return _initialized;
            }
        }
    }

    /// <summary>
    /// Initializes the underlying native TA-Lib library. Invocation is idempotent.
    /// </summary>
    public static void Initialize()
    {
        lock (SyncRoot)
        {
            if (_initialized)
            {
                return;
            }

            var retCode = TaLibNative.Initialize();
            if (retCode != TaRetCode.Success)
            {
                TaLibException.Throw(retCode, "initializing TA-Lib");
            }

            _initialized = true;
        }
    }

    /// <summary>
    /// Shuts down the native TA-Lib library and releases global resources.
    /// </summary>
    public static void Shutdown()
    {
        lock (SyncRoot)
        {
            if (!_initialized)
            {
                return;
            }

            var retCode = TaLibNative.Shutdown();
            if (retCode != TaRetCode.Success)
            {
                TaLibException.Throw(retCode, "shutting down TA-Lib");
            }

            _initialized = false;
        }
    }

    /// <summary>
    /// Gets the version string reported by the native TA-Lib binary.
    /// </summary>
    public static string GetVersionString()
    {
        EnsureInitialized();
        return TaLibNativeLibrary.PtrToStringAnsi(TaLibNative.GetVersionString());
    }

    /// <summary>
    /// Calculates a simple moving average (SMA) over the supplied data.
    /// </summary>
    /// <param name="values">Input data points.</param>
    /// <param name="period">Window length for the moving average.</param>
    /// <returns>An array containing the SMA values. The first <paramref name="period"/> - 1 items are omitted.</returns>
    /// <exception cref="ArgumentNullException">Thrown when <paramref name="values"/> is null.</exception>
    /// <exception cref="ArgumentOutOfRangeException">Thrown when <paramref name="period"/> is less than 1.</exception>
    public static double[] CalculateSimpleMovingAverage(double[] values, int period)
    {
        if (values is null)
        {
            throw new ArgumentNullException(nameof(values));
        }

        if (period < 1)
        {
            throw new ArgumentOutOfRangeException(nameof(period), period, "Period must be at least 1.");
        }

        if (values.Length == 0)
        {
            return Array.Empty<double>();
        }

        EnsureInitialized();

        var output = new double[values.Length];
        var retCode = TaLibNative.SimpleMovingAverage(
            0,
            values.Length - 1,
            values,
            period,
            out var outBegIdx,
            out var outNbElement,
            output);

        if (retCode != TaRetCode.Success)
        {
            TaLibException.Throw(retCode, "computing simple moving average");
        }

        return SliceOutput(output, outBegIdx, outNbElement);
    }

    /// <summary>
    /// Calculates a moving average using the algorithm specified by <paramref name="type"/>.
    /// </summary>
    /// <param name="values">Input data points.</param>
    /// <param name="period">Window length for the moving average.</param>
    /// <param name="type">The moving average type to compute.</param>
    /// <returns>An array containing the moving average values.</returns>
    /// <exception cref="ArgumentNullException">Thrown when <paramref name="values"/> is null.</exception>
    /// <exception cref="ArgumentOutOfRangeException">Thrown when <paramref name="period"/> is less than 1.</exception>
    public static double[] CalculateMovingAverage(double[] values, int period, MovingAverageType type)
    {
        if (values is null)
        {
            throw new ArgumentNullException(nameof(values));
        }

        if (period < 1)
        {
            throw new ArgumentOutOfRangeException(nameof(period), period, "Period must be at least 1.");
        }

        if (values.Length == 0)
        {
            return Array.Empty<double>();
        }

        EnsureInitialized();

        var output = new double[values.Length];
        var retCode = TaLibNative.MovingAverage(
            0,
            values.Length - 1,
            values,
            period,
            type,
            out var outBegIdx,
            out var outNbElement,
            output);

        if (retCode != TaRetCode.Success)
        {
            TaLibException.Throw(retCode, "computing moving average");
        }

        return SliceOutput(output, outBegIdx, outNbElement);
    }

    /// <summary>
    /// Calculates the relative strength index (RSI) over the supplied data.
    /// </summary>
    /// <param name="values">Input data points.</param>
    /// <param name="period">Lookback length for RSI.</param>
    /// <returns>The RSI output series.</returns>
    public static double[] CalculateRelativeStrengthIndex(double[] values, int period)
    {
        if (values is null)
        {
            throw new ArgumentNullException(nameof(values));
        }

        if (period < 1)
        {
            throw new ArgumentOutOfRangeException(nameof(period), period, "Period must be at least 1.");
        }

        if (values.Length == 0)
        {
            return Array.Empty<double>();
        }

        EnsureInitialized();

        var output = new double[values.Length];
        var retCode = TaLibNative.RelativeStrengthIndex(
            0,
            values.Length - 1,
            values,
            period,
            out var outBegIdx,
            out var outNbElement,
            output);

        if (retCode != TaRetCode.Success)
        {
            TaLibException.Throw(retCode, "computing RSI");
        }

        return SliceOutput(output, outBegIdx, outNbElement);
    }

    /// <summary>
    /// Calculates the Moving Average Convergence Divergence (MACD) indicator.
    /// </summary>
    /// <param name="values">Input data points.</param>
    /// <param name="fastPeriod">Number of periods for the fast EMA.</param>
    /// <param name="slowPeriod">Number of periods for the slow EMA.</param>
    /// <param name="signalPeriod">Number of periods for the signal line EMA.</param>
    /// <returns>MACD line, signal line, and histogram arrays.</returns>
    public static MacdResult CalculateMacd(double[] values, int fastPeriod, int slowPeriod, int signalPeriod)
    {
        if (values is null)
        {
            throw new ArgumentNullException(nameof(values));
        }

        ValidatePositivePeriod(fastPeriod, nameof(fastPeriod));
        ValidatePositivePeriod(slowPeriod, nameof(slowPeriod));
        ValidatePositivePeriod(signalPeriod, nameof(signalPeriod));

        if (values.Length == 0)
        {
            return new MacdResult(Array.Empty<double>(), Array.Empty<double>(), Array.Empty<double>());
        }

        EnsureInitialized();

        var macd = new double[values.Length];
        var signal = new double[values.Length];
        var hist = new double[values.Length];

        var retCode = TaLibNative.MovingAverageConvergenceDivergence(
            0,
            values.Length - 1,
            values,
            fastPeriod,
            slowPeriod,
            signalPeriod,
            out var outBegIdx,
            out var outNbElement,
            macd,
            signal,
            hist);

        if (retCode != TaRetCode.Success)
        {
            TaLibException.Throw(retCode, "computing MACD");
        }

        return new MacdResult(
            SliceOutput(macd, outBegIdx, outNbElement),
            SliceOutput(signal, outBegIdx, outNbElement),
            SliceOutput(hist, outBegIdx, outNbElement));
    }

    /// <summary>
    /// Gets the lookback period for the simple moving average (number of warm-up samples).
    /// </summary>
    public static int GetSimpleMovingAverageLookback(int period)
    {
        if (period < 1)
        {
            throw new ArgumentOutOfRangeException(nameof(period), period, "Period must be at least 1.");
        }

        return TaLibNative.SimpleMovingAverageLookback(period);
    }

    /// <summary>
    /// Gets the lookback period for the generic TA_MA function.
    /// </summary>
    public static int GetMovingAverageLookback(int period)
    {
        if (period < 1)
        {
            throw new ArgumentOutOfRangeException(nameof(period), period, "Period must be at least 1.");
        }

        return TaLibNative.MovingAverageLookback(period);
    }

    /// <summary>
    /// Gets the lookback period for the RSI indicator.
    /// </summary>
    public static int GetRelativeStrengthIndexLookback(int period)
    {
        if (period < 1)
        {
            throw new ArgumentOutOfRangeException(nameof(period), period, "Period must be at least 1.");
        }

        return TaLibNative.RelativeStrengthIndexLookback(period);
    }

    /// <summary>
    /// Gets the lookback period for the MACD indicator.
    /// </summary>
    public static int GetMacdLookback(int fastPeriod, int slowPeriod, int signalPeriod)
    {
        ValidatePositivePeriod(fastPeriod, nameof(fastPeriod));
        ValidatePositivePeriod(slowPeriod, nameof(slowPeriod));
        ValidatePositivePeriod(signalPeriod, nameof(signalPeriod));

        return TaLibNative.MovingAverageConvergenceDivergenceLookback(fastPeriod, slowPeriod, signalPeriod);
    }

    private static void EnsureInitialized()
    {
        if (!IsInitialized)
        {
            Initialize();
        }
    }

    private static double[] SliceOutput(double[] buffer, int outBegIdx, int outNbElement)
    {
        if (outNbElement <= 0)
        {
            return Array.Empty<double>();
        }

        var result = new double[outNbElement];
        Array.Copy(buffer, 0, result, 0, outNbElement);
        return result;
    }

    private static void ValidatePositivePeriod(int value, string paramName)
    {
        if (value < 1)
        {
            throw new ArgumentOutOfRangeException(paramName, value, "Period must be at least 1.");
        }
    }
}
