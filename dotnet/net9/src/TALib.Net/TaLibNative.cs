using System.Runtime.InteropServices;

namespace TALib.Net;

internal static class TaLibNative
{
    static TaLibNative()
    {
        TaLibNativeLibrary.EnsureResolver();
    }

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_Initialize", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode Initialize();

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_Shutdown", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode Shutdown();

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_GetVersionString", CallingConvention = CallingConvention.Cdecl)]
    internal static extern IntPtr GetVersionString();

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_MA", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode MovingAverage(
        int startIdx,
        int endIdx,
        [In] double[] inReal,
        int optInTimePeriod,
        MovingAverageType optInMAType,
        out int outBegIdx,
        out int outNbElement,
        [Out] double[] outReal);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_MA_Lookback", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int MovingAverageLookback(int optInTimePeriod);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_SMA", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode SimpleMovingAverage(
        int startIdx,
        int endIdx,
        [In] double[] inReal,
        int optInTimePeriod,
        out int outBegIdx,
        out int outNbElement,
        [Out] double[] outReal);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_SMA_Lookback", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int SimpleMovingAverageLookback(int optInTimePeriod);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_RSI", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode RelativeStrengthIndex(
        int startIdx,
        int endIdx,
        [In] double[] inReal,
        int optInTimePeriod,
        out int outBegIdx,
        out int outNbElement,
        [Out] double[] outReal);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_RSI_Lookback", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int RelativeStrengthIndexLookback(int optInTimePeriod);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_MACD", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode MovingAverageConvergenceDivergence(
        int startIdx,
        int endIdx,
        [In] double[] inReal,
        int optInFastPeriod,
        int optInSlowPeriod,
        int optInSignalPeriod,
        out int outBegIdx,
        out int outNbElement,
        [Out] double[] outMacd,
        [Out] double[] outSignal,
        [Out] double[] outHist);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_MACD_Lookback", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int MovingAverageConvergenceDivergenceLookback(
        int optInFastPeriod,
        int optInSlowPeriod,
        int optInSignalPeriod);
}
