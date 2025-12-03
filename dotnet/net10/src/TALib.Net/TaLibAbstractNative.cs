using System;
using System.Runtime.InteropServices;

namespace TALib.Net;

internal static class TaLibAbstractNative
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct TaStringTable
    {
        internal uint Size;
        internal IntPtr Strings;
        internal IntPtr HiddenData;
    }

    [Flags]
    internal enum TaFuncFlags
    {
        None = 0,
        Overlap = 0x01000000,
        Volume = 0x04000000,
        UnstablePeriod = 0x08000000,
        Candlestick = 0x10000000,
    }

    internal enum TaInputParameterType
    {
        Price,
        Real,
        Integer,
    }

    [Flags]
    internal enum TaInputFlags
    {
        None = 0,
        PriceOpen = 0x00000001,
        PriceHigh = 0x00000002,
        PriceLow = 0x00000004,
        PriceClose = 0x00000008,
        PriceVolume = 0x00000010,
        PriceOpenInterest = 0x00000020,
        PriceTimestamp = 0x00000040,
    }

    internal enum TaOptInputParameterType
    {
        RealRange,
        RealList,
        IntegerRange,
        IntegerList,
    }

    [Flags]
    internal enum TaOptInputFlags
    {
        None = 0,
        IsPercent = 0x00100000,
        IsDegree = 0x00200000,
        IsCurrency = 0x00400000,
        Advanced = 0x01000000,
    }

    internal enum TaOutputParameterType
    {
        Real,
        Integer,
    }

    [Flags]
    internal enum TaOutputFlags
    {
        None = 0,
        Line = 0x00000001,
        DotLine = 0x00000002,
        DashLine = 0x00000004,
        Dot = 0x00000008,
        Histogram = 0x00000010,
        PatternBool = 0x00000020,
        PatternBullBear = 0x00000040,
        PatternStrength = 0x00000080,
        Positive = 0x00000100,
        Negative = 0x00000200,
        Zero = 0x00000400,
        UpperLimit = 0x00000800,
        LowerLimit = 0x00001000,
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TaFuncInfoNative
    {
        internal IntPtr Name;
        internal IntPtr Group;
        internal IntPtr Hint;
        internal IntPtr CamelCaseName;
        internal TaFuncFlags Flags;
        internal uint NbInput;
        internal uint NbOptInput;
        internal uint NbOutput;
        internal IntPtr Handle;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TaInputParameterInfoNative
    {
        internal TaInputParameterType Type;
        internal IntPtr ParamName;
        internal TaInputFlags Flags;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TaOptInputParameterInfoNative
    {
        internal TaOptInputParameterType Type;
        internal IntPtr ParamName;
        internal TaOptInputFlags Flags;
        internal IntPtr DisplayName;
        internal IntPtr DataSet;
        internal double DefaultValue;
        internal IntPtr Hint;
        internal IntPtr HelpFile;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TaOutputParameterInfoNative
    {
        internal TaOutputParameterType Type;
        internal IntPtr ParamName;
        internal TaOutputFlags Flags;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TaRealRangeNative
    {
        internal double Min;
        internal double Max;
        internal int Precision;
        internal double SuggestedStart;
        internal double SuggestedEnd;
        internal double SuggestedIncrement;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TaIntegerRangeNative
    {
        internal int Min;
        internal int Max;
        internal int SuggestedStart;
        internal int SuggestedEnd;
        internal int SuggestedIncrement;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TaRealDataPairNative
    {
        internal double Value;
        internal IntPtr String;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TaIntegerDataPairNative
    {
        internal int Value;
        internal IntPtr String;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TaRealListNative
    {
        internal IntPtr Data;
        internal uint NbElement;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct TaIntegerListNative
    {
        internal IntPtr Data;
        internal uint NbElement;
    }

    internal sealed class SafeTaParamHolderHandle : SafeHandle
    {
        private SafeTaParamHolderHandle()
            : base(IntPtr.Zero, ownsHandle: true)
        {
        }

        internal static SafeTaParamHolderHandle FromPtr(IntPtr ptr)
        {
            var handle = new SafeTaParamHolderHandle();
            handle.SetHandle(ptr);
            return handle;
        }

        public override bool IsInvalid => handle == IntPtr.Zero;

        protected override bool ReleaseHandle()
        {
            var ret = ParamHolderFree(handle);
            SetHandle(IntPtr.Zero);
            return ret == TaRetCode.Success;
        }
    }

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_GroupTableAlloc", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode GroupTableAlloc(out IntPtr tablePtr);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_GroupTableFree", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode GroupTableFree(IntPtr tablePtr);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_FuncTableAlloc", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode FuncTableAlloc([MarshalAs(UnmanagedType.LPStr)] string group, out IntPtr tablePtr);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_FuncTableFree", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode FuncTableFree(IntPtr tablePtr);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_GetFuncHandle", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode GetFuncHandle([MarshalAs(UnmanagedType.LPStr)] string name, out IntPtr handlePtr);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_GetFuncInfo", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode GetFuncInfo(IntPtr handlePtr, out IntPtr infoPtr);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_GetInputParameterInfo", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode GetInputParameterInfo(IntPtr handlePtr, uint paramIndex, out IntPtr infoPtr);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_GetOptInputParameterInfo", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode GetOptInputParameterInfo(IntPtr handlePtr, uint paramIndex, out IntPtr infoPtr);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_GetOutputParameterInfo", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode GetOutputParameterInfo(IntPtr handlePtr, uint paramIndex, out IntPtr infoPtr);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_ParamHolderAlloc", CallingConvention = CallingConvention.Cdecl)]
    private static extern TaRetCode ParamHolderAllocInternal(IntPtr handlePtr, out IntPtr paramsPtr);

    internal static TaRetCode ParamHolderAlloc(IntPtr handlePtr, out SafeTaParamHolderHandle handle)
    {
        var ret = ParamHolderAllocInternal(handlePtr, out var ptr);
        handle = ret == TaRetCode.Success && ptr != IntPtr.Zero
            ? SafeTaParamHolderHandle.FromPtr(ptr)
            : SafeTaParamHolderHandle.FromPtr(IntPtr.Zero);

        return ret;
    }

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_ParamHolderFree", CallingConvention = CallingConvention.Cdecl)]
    private static extern TaRetCode ParamHolderFreeInternal(IntPtr paramsPtr);

    internal static TaRetCode ParamHolderFree(IntPtr paramsPtr) => paramsPtr == IntPtr.Zero
        ? TaRetCode.Success
        : ParamHolderFreeInternal(paramsPtr);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_SetInputParamIntegerPtr", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode SetInputParamIntegerPtr(SafeTaParamHolderHandle holder, uint paramIndex, int[] values);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_SetInputParamRealPtr", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode SetInputParamRealPtr(SafeTaParamHolderHandle holder, uint paramIndex, double[] values);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_SetInputParamPricePtr", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode SetInputParamPricePtr(
        SafeTaParamHolderHandle holder,
        uint paramIndex,
        double[]? open,
        double[]? high,
        double[]? low,
        double[]? close,
        double[]? volume,
        double[]? openInterest);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_SetOptInputParamInteger", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode SetOptInputParamInteger(SafeTaParamHolderHandle holder, uint paramIndex, int value);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_SetOptInputParamReal", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode SetOptInputParamReal(SafeTaParamHolderHandle holder, uint paramIndex, double value);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_SetOutputParamIntegerPtr", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode SetOutputParamIntegerPtr(SafeTaParamHolderHandle holder, uint paramIndex, int[] output);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_SetOutputParamRealPtr", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode SetOutputParamRealPtr(SafeTaParamHolderHandle holder, uint paramIndex, double[] output);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_GetLookback", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode GetLookback(SafeTaParamHolderHandle holder, out int lookback);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_CallFunc", CallingConvention = CallingConvention.Cdecl)]
    internal static extern TaRetCode CallFunc(
        SafeTaParamHolderHandle holder,
        int startIdx,
        int endIdx,
        out int outBegIdx,
        out int outNbElement);

    [DllImport(TaLibNativeLibrary.LibraryName, EntryPoint = "TA_FunctionDescriptionXML", CallingConvention = CallingConvention.Cdecl)]
    internal static extern IntPtr GetFunctionDescriptionXml();
}
