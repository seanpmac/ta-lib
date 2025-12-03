using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace TALib.Net;

/// <summary>
/// Flags attached to a TA function.
/// </summary>
[Flags]
public enum TaFunctionFlags
{
    None = 0,
    Overlap = (int)TaLibAbstractNative.TaFuncFlags.Overlap,
    Volume = (int)TaLibAbstractNative.TaFuncFlags.Volume,
    UnstablePeriod = (int)TaLibAbstractNative.TaFuncFlags.UnstablePeriod,
    Candlestick = (int)TaLibAbstractNative.TaFuncFlags.Candlestick,
}

/// <summary>
/// Describes the type of an input parameter.
/// </summary>
public enum TaInputParameterType
{
    Price = TaLibAbstractNative.TaInputParameterType.Price,
    Real = TaLibAbstractNative.TaInputParameterType.Real,
    Integer = TaLibAbstractNative.TaInputParameterType.Integer,
}

/// <summary>
/// Flags describing which price components are required for a price input.
/// </summary>
[Flags]
public enum TaInputFlags
{
    None = 0,
    PriceOpen = (int)TaLibAbstractNative.TaInputFlags.PriceOpen,
    PriceHigh = (int)TaLibAbstractNative.TaInputFlags.PriceHigh,
    PriceLow = (int)TaLibAbstractNative.TaInputFlags.PriceLow,
    PriceClose = (int)TaLibAbstractNative.TaInputFlags.PriceClose,
    PriceVolume = (int)TaLibAbstractNative.TaInputFlags.PriceVolume,
    PriceOpenInterest = (int)TaLibAbstractNative.TaInputFlags.PriceOpenInterest,
    PriceTimestamp = (int)TaLibAbstractNative.TaInputFlags.PriceTimestamp,
}

/// <summary>
/// Describes the kind of optional input.
/// </summary>
public enum TaOptInputParameterType
{
    RealRange = TaLibAbstractNative.TaOptInputParameterType.RealRange,
    RealList = TaLibAbstractNative.TaOptInputParameterType.RealList,
    IntegerRange = TaLibAbstractNative.TaOptInputParameterType.IntegerRange,
    IntegerList = TaLibAbstractNative.TaOptInputParameterType.IntegerList,
}

/// <summary>
/// Flags providing hints about an optional input parameter.
/// </summary>
[Flags]
public enum TaOptInputFlags
{
    None = 0,
    IsPercent = (int)TaLibAbstractNative.TaOptInputFlags.IsPercent,
    IsDegree = (int)TaLibAbstractNative.TaOptInputFlags.IsDegree,
    IsCurrency = (int)TaLibAbstractNative.TaOptInputFlags.IsCurrency,
    Advanced = (int)TaLibAbstractNative.TaOptInputFlags.Advanced,
}

/// <summary>
/// Output types exposed by TA-Lib.
/// </summary>
public enum TaOutputParameterType
{
    Real = TaLibAbstractNative.TaOutputParameterType.Real,
    Integer = TaLibAbstractNative.TaOutputParameterType.Integer,
}

/// <summary>
/// Display hints exposed for TA-Lib outputs.
/// </summary>
[Flags]
public enum TaOutputFlags
{
    None = 0,
    Line = (int)TaLibAbstractNative.TaOutputFlags.Line,
    DotLine = (int)TaLibAbstractNative.TaOutputFlags.DotLine,
    DashLine = (int)TaLibAbstractNative.TaOutputFlags.DashLine,
    Dot = (int)TaLibAbstractNative.TaOutputFlags.Dot,
    Histogram = (int)TaLibAbstractNative.TaOutputFlags.Histogram,
    PatternBool = (int)TaLibAbstractNative.TaOutputFlags.PatternBool,
    PatternBullBear = (int)TaLibAbstractNative.TaOutputFlags.PatternBullBear,
    PatternStrength = (int)TaLibAbstractNative.TaOutputFlags.PatternStrength,
    Positive = (int)TaLibAbstractNative.TaOutputFlags.Positive,
    Negative = (int)TaLibAbstractNative.TaOutputFlags.Negative,
    Zero = (int)TaLibAbstractNative.TaOutputFlags.Zero,
    UpperLimit = (int)TaLibAbstractNative.TaOutputFlags.UpperLimit,
    LowerLimit = (int)TaLibAbstractNative.TaOutputFlags.LowerLimit,
}

/// <summary>
/// Describes a named numeric value for an optional input list.
/// </summary>
public sealed record TaValueLabel(double Value, string Label);

/// <summary>
/// Describes a named integer value for an optional input list.
/// </summary>
public sealed record TaIntegerValueLabel(int Value, string Label);

/// <summary>
/// Represents range metadata for a real-valued optional input.
/// </summary>
public sealed record TaRealRange(double Min, double Max, int Precision, double SuggestedStart, double SuggestedEnd, double SuggestedIncrement);

/// <summary>
/// Represents range metadata for an integer optional input.
/// </summary>
public sealed record TaIntegerRange(int Min, int Max, int SuggestedStart, int SuggestedEnd, int SuggestedIncrement);

/// <summary>
/// Metadata describing a single input parameter.
/// </summary>
public sealed record TaInputParameterInfo(string Name, TaInputParameterType Type, TaInputFlags Flags);

/// <summary>
/// Metadata describing an optional input parameter.
/// </summary>
public sealed record TaOptInputParameterInfo(
    string Name,
    TaOptInputParameterType Type,
    TaOptInputFlags Flags,
    string? DisplayName,
    double DefaultValue,
    string? Hint,
    string? HelpFile,
    TaRealRange? RealRange,
    IReadOnlyList<TaValueLabel>? RealList,
    TaIntegerRange? IntegerRange,
    IReadOnlyList<TaIntegerValueLabel>? IntegerList);

/// <summary>
/// Metadata describing a single output parameter.
/// </summary>
public sealed record TaOutputParameterInfo(string Name, TaOutputParameterType Type, TaOutputFlags Flags);

/// <summary>
/// Provides read-only metadata about a TA-Lib function.
/// </summary>
public sealed class TaFunctionDescriptor
{
    internal TaFunctionDescriptor(IntPtr handle, string name, string group, string? hint, string? camelCaseName, TaFunctionFlags flags, IReadOnlyList<TaInputParameterInfo> inputs, IReadOnlyList<TaOptInputParameterInfo> optionalInputs, IReadOnlyList<TaOutputParameterInfo> outputs)
    {
        Handle = handle;
        Name = name;
        Group = group;
        Hint = hint;
        CamelCaseName = camelCaseName;
        Flags = flags;
        Inputs = inputs;
        OptionalInputs = optionalInputs;
        Outputs = outputs;
    }

    internal IntPtr Handle { get; }

    /// <summary>
    /// Gets the canonical function name (for example "RSI").
    /// </summary>
    public string Name { get; }

    /// <summary>
    /// Gets the group under which TA-Lib categorises the function.
    /// </summary>
    public string Group { get; }

    /// <summary>
    /// Gets the descriptive hint associated with the function, if any.
    /// </summary>
    public string? Hint { get; }

    /// <summary>
    /// Gets the camel-case version of the function name.
    /// </summary>
    public string? CamelCaseName { get; }

    /// <summary>
    /// Gets function-level flags.
    /// </summary>
    public TaFunctionFlags Flags { get; }

    /// <summary>
    /// Gets the required input parameters.
    /// </summary>
    public IReadOnlyList<TaInputParameterInfo> Inputs { get; }

    /// <summary>
    /// Gets the optional input parameters.
    /// </summary>
    public IReadOnlyList<TaOptInputParameterInfo> OptionalInputs { get; }

    /// <summary>
    /// Gets the output parameters.
    /// </summary>
    public IReadOnlyList<TaOutputParameterInfo> Outputs { get; }

    /// <summary>
    /// Creates a mutable function invocation that can be populated and executed.
    /// </summary>
    public TaFunctionInvocation CreateInvocation() => TaFunctionInvocation.Create(this);
}

/// <summary>
/// Represents a pending invocation of a TA-Lib function.
/// </summary>
public sealed class TaFunctionInvocation : IDisposable
{
    private readonly TaFunctionDescriptor _descriptor;
    private readonly TaLibAbstractNative.SafeTaParamHolderHandle _handle;
    private bool _disposed;

    private TaFunctionInvocation(TaFunctionDescriptor descriptor, TaLibAbstractNative.SafeTaParamHolderHandle handle)
    {
        _descriptor = descriptor;
        _handle = handle;
    }

    internal static TaFunctionInvocation Create(TaFunctionDescriptor descriptor)
    {
        TaLibCore.Initialize();
        var ret = TaLibAbstractNative.ParamHolderAlloc(descriptor.Handle, out var handle);
        if (ret != TaRetCode.Success || handle.IsInvalid)
        {
            throw new TaLibException(ret, $"Failed to allocate parameter holder for {descriptor.Name}.");
        }

        return new TaFunctionInvocation(descriptor, handle);
    }

    /// <summary>
    /// Sets a real-valued input series.
    /// </summary>
    public void SetInputReal(int index, double[] values)
    {
        EnsureNotDisposed();
        ArgumentOutOfRangeException.ThrowIfNegative(index);
        ArgumentNullException.ThrowIfNull(values);
        var ret = TaLibAbstractNative.SetInputParamRealPtr(_handle, (uint)index, values);
        if (ret != TaRetCode.Success)
        {
            TaLibException.Throw(ret, $"setting real input {index} on {_descriptor.Name}");
        }
    }

    /// <summary>
    /// Sets an integer-valued input series.
    /// </summary>
    public void SetInputInteger(int index, int[] values)
    {
        EnsureNotDisposed();
        ArgumentOutOfRangeException.ThrowIfNegative(index);
        ArgumentNullException.ThrowIfNull(values);
        var ret = TaLibAbstractNative.SetInputParamIntegerPtr(_handle, (uint)index, values);
        if (ret != TaRetCode.Success)
        {
            TaLibException.Throw(ret, $"setting integer input {index} on {_descriptor.Name}");
        }
    }

    /// <summary>
    /// Sets a price-series input, providing the required components.
    /// </summary>
    public void SetInputPrice(int index, double[]? open, double[]? high, double[]? low, double[]? close, double[]? volume, double[]? openInterest)
    {
        EnsureNotDisposed();
        ArgumentOutOfRangeException.ThrowIfNegative(index);
        var ret = TaLibAbstractNative.SetInputParamPricePtr(_handle, (uint)index, open, high, low, close, volume, openInterest);
        if (ret != TaRetCode.Success)
        {
            TaLibException.Throw(ret, $"setting price input {index} on {_descriptor.Name}");
        }
    }

    /// <summary>
    /// Sets a real-valued optional input.
    /// </summary>
    public void SetOptionalReal(int index, double value)
    {
        EnsureNotDisposed();
        ArgumentOutOfRangeException.ThrowIfNegative(index);
        var ret = TaLibAbstractNative.SetOptInputParamReal(_handle, (uint)index, value);
        if (ret != TaRetCode.Success)
        {
            TaLibException.Throw(ret, $"setting optional real input {index} on {_descriptor.Name}");
        }
    }

    /// <summary>
    /// Sets an integer-valued optional input.
    /// </summary>
    public void SetOptionalInteger(int index, int value)
    {
        EnsureNotDisposed();
        ArgumentOutOfRangeException.ThrowIfNegative(index);
        var ret = TaLibAbstractNative.SetOptInputParamInteger(_handle, (uint)index, value);
        if (ret != TaRetCode.Success)
        {
            TaLibException.Throw(ret, $"setting optional integer input {index} on {_descriptor.Name}");
        }
    }

    /// <summary>
    /// Assigns a real-valued output buffer.
    /// </summary>
    public void SetOutputReal(int index, double[] destination)
    {
        EnsureNotDisposed();
        ArgumentOutOfRangeException.ThrowIfNegative(index);
        ArgumentNullException.ThrowIfNull(destination);
        var ret = TaLibAbstractNative.SetOutputParamRealPtr(_handle, (uint)index, destination);
        if (ret != TaRetCode.Success)
        {
            TaLibException.Throw(ret, $"setting real output {index} on {_descriptor.Name}");
        }
    }

    /// <summary>
    /// Assigns an integer-valued output buffer.
    /// </summary>
    public void SetOutputInteger(int index, int[] destination)
    {
        EnsureNotDisposed();
        ArgumentOutOfRangeException.ThrowIfNegative(index);
        ArgumentNullException.ThrowIfNull(destination);
        var ret = TaLibAbstractNative.SetOutputParamIntegerPtr(_handle, (uint)index, destination);
        if (ret != TaRetCode.Success)
        {
            TaLibException.Throw(ret, $"setting integer output {index} on {_descriptor.Name}");
        }
    }

    /// <summary>
    /// Queries the lookback period implied by the current optional input configuration.
    /// </summary>
    public int GetLookback()
    {
        EnsureNotDisposed();
        var ret = TaLibAbstractNative.GetLookback(_handle, out var lookback);
        if (ret != TaRetCode.Success)
        {
            TaLibException.Throw(ret, $"retrieving lookback for {_descriptor.Name}");
        }

        return lookback;
    }

    /// <summary>
    /// Executes the configured function and returns the starting index and element count for the outputs.
    /// </summary>
    public TaFunctionResult Execute(int startIdx, int endIdx)
    {
        EnsureNotDisposed();
        var ret = TaLibAbstractNative.CallFunc(_handle, startIdx, endIdx, out var outBegIdx, out var outNbElement);
        if (ret != TaRetCode.Success)
        {
            TaLibException.Throw(ret, $"executing {_descriptor.Name}");
        }

        return new TaFunctionResult(outBegIdx, outNbElement);
    }

    private void EnsureNotDisposed()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
    }

    /// <inheritdoc />
    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _handle.Dispose();
        _disposed = true;
        GC.SuppressFinalize(this);
    }
}

/// <summary>
/// Result metadata returned when invoking a TA function.
/// </summary>
/// <param name="StartIndex">Index within the input series corresponding to the first output element.</param>
/// <param name="ElementCount">Number of populated elements in each output buffer.</param>
public readonly record struct TaFunctionResult(int StartIndex, int ElementCount);

/// <summary>
/// Provides access to the full TA-Lib function catalogue and metadata.
/// </summary>
public static class TaFunctionCatalog
{
    /// <summary>
    /// Lists all available function groups.
    /// </summary>
    public static IReadOnlyList<string> GetGroups()
    {
        TaLibCore.Initialize();
        var ret = TaLibAbstractNative.GroupTableAlloc(out var tablePtr);
        if (ret != TaRetCode.Success)
        {
            TaLibException.Throw(ret, "allocating TA group table");
        }

        if (tablePtr == IntPtr.Zero)
        {
            return Array.Empty<string>();
        }

        try
        {
            var table = Marshal.PtrToStructure<TaLibAbstractNative.TaStringTable>(tablePtr);
            if (table.Size == 0 || table.Strings == IntPtr.Zero)
            {
                return Array.Empty<string>();
            }

            var result = new string[table.Size];
            for (var i = 0; i < result.Length; i++)
            {
                var ptr = Marshal.ReadIntPtr(table.Strings, IntPtr.Size * i);
                result[i] = TaLibNativeLibrary.PtrToStringAnsi(ptr);
            }

            return result;
        }
        finally
        {
            TaLibAbstractNative.GroupTableFree(tablePtr);
        }
    }

    /// <summary>
    /// Lists all function names belonging to the specified group.
    /// </summary>
    public static IReadOnlyList<string> GetFunctionNames(string group)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(group);
        TaLibCore.Initialize();

        var ret = TaLibAbstractNative.FuncTableAlloc(group, out var tablePtr);
        if (ret != TaRetCode.Success)
        {
            TaLibException.Throw(ret, $"allocating TA function table for group '{group}'");
        }

        if (tablePtr == IntPtr.Zero)
        {
            return Array.Empty<string>();
        }

        try
        {
            var table = Marshal.PtrToStructure<TaLibAbstractNative.TaStringTable>(tablePtr);
            if (table.Size == 0 || table.Strings == IntPtr.Zero)
            {
                return Array.Empty<string>();
            }

            var result = new string[table.Size];
            for (var i = 0; i < result.Length; i++)
            {
                var ptr = Marshal.ReadIntPtr(table.Strings, IntPtr.Size * i);
                result[i] = TaLibNativeLibrary.PtrToStringAnsi(ptr);
            }

            return result;
        }
        finally
        {
            TaLibAbstractNative.FuncTableFree(tablePtr);
        }
    }

    /// <summary>
    /// Retrieves detailed metadata for the specified function name.
    /// </summary>
    public static TaFunctionDescriptor GetFunction(string name)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(name);
        TaLibCore.Initialize();

        var ret = TaLibAbstractNative.GetFuncHandle(name, out var handlePtr);
        if (ret != TaRetCode.Success)
        {
            TaLibException.Throw(ret, $"getting function handle for '{name}'");
        }

        ret = TaLibAbstractNative.GetFuncInfo(handlePtr, out var infoPtr);
        if (ret != TaRetCode.Success)
        {
            TaLibException.Throw(ret, $"getting function info for '{name}'");
        }

        if (infoPtr == IntPtr.Zero)
        {
            throw new TaLibException(TaRetCode.FuncNotFound, $"Function '{name}' metadata is unavailable.");
        }

        var info = Marshal.PtrToStructure<TaLibAbstractNative.TaFuncInfoNative>(infoPtr);
        var functionName = TaLibNativeLibrary.PtrToStringAnsi(info.Name);
        var group = TaLibNativeLibrary.PtrToStringAnsi(info.Group);
        var hint = TaLibNativeLibrary.PtrToStringAnsi(info.Hint);
        var camelCaseName = TaLibNativeLibrary.PtrToStringAnsi(info.CamelCaseName);

        var inputs = new List<TaInputParameterInfo>((int)info.NbInput);
        for (uint i = 0; i < info.NbInput; i++)
        {
            ret = TaLibAbstractNative.GetInputParameterInfo(handlePtr, i, out var paramPtr);
            if (ret != TaRetCode.Success)
            {
                TaLibException.Throw(ret, $"getting input parameter info {i} for '{name}'");
            }

            var native = Marshal.PtrToStructure<TaLibAbstractNative.TaInputParameterInfoNative>(paramPtr);
            inputs.Add(new TaInputParameterInfo(
                TaLibNativeLibrary.PtrToStringAnsi(native.ParamName),
                (TaInputParameterType)native.Type,
                (TaInputFlags)native.Flags));
        }

        var optInputs = new List<TaOptInputParameterInfo>((int)info.NbOptInput);
        for (uint i = 0; i < info.NbOptInput; i++)
        {
            ret = TaLibAbstractNative.GetOptInputParameterInfo(handlePtr, i, out var paramPtr);
            if (ret != TaRetCode.Success)
            {
                TaLibException.Throw(ret, $"getting optional parameter info {i} for '{name}'");
            }

            var native = Marshal.PtrToStructure<TaLibAbstractNative.TaOptInputParameterInfoNative>(paramPtr);
            var optName = TaLibNativeLibrary.PtrToStringAnsi(native.ParamName);
            var displayName = TaLibNativeLibrary.PtrToStringAnsi(native.DisplayName);
            var hintText = TaLibNativeLibrary.PtrToStringAnsi(native.Hint);
            var helpFile = TaLibNativeLibrary.PtrToStringAnsi(native.HelpFile);

            TaRealRange? realRange = null;
            IReadOnlyList<TaValueLabel>? realList = null;
            TaIntegerRange? intRange = null;
            IReadOnlyList<TaIntegerValueLabel>? intList = null;

            switch (native.Type)
            {
                case TaLibAbstractNative.TaOptInputParameterType.RealRange:
                    if (native.DataSet != IntPtr.Zero)
                    {
                        var range = Marshal.PtrToStructure<TaLibAbstractNative.TaRealRangeNative>(native.DataSet);
                        realRange = new TaRealRange(range.Min, range.Max, range.Precision, range.SuggestedStart, range.SuggestedEnd, range.SuggestedIncrement);
                    }
                    break;
                case TaLibAbstractNative.TaOptInputParameterType.RealList:
                    realList = ReadRealList(native.DataSet);
                    break;
                case TaLibAbstractNative.TaOptInputParameterType.IntegerRange:
                    if (native.DataSet != IntPtr.Zero)
                    {
                        var range = Marshal.PtrToStructure<TaLibAbstractNative.TaIntegerRangeNative>(native.DataSet);
                        intRange = new TaIntegerRange(range.Min, range.Max, range.SuggestedStart, range.SuggestedEnd, range.SuggestedIncrement);
                    }
                    break;
                case TaLibAbstractNative.TaOptInputParameterType.IntegerList:
                    intList = ReadIntegerList(native.DataSet);
                    break;
            }

            optInputs.Add(new TaOptInputParameterInfo(
                optName,
                (TaOptInputParameterType)native.Type,
                (TaOptInputFlags)native.Flags,
                displayName,
                native.DefaultValue,
                hintText,
                helpFile,
                realRange,
                realList,
                intRange,
                intList));
        }

        var outputs = new List<TaOutputParameterInfo>((int)info.NbOutput);
        for (uint i = 0; i < info.NbOutput; i++)
        {
            ret = TaLibAbstractNative.GetOutputParameterInfo(handlePtr, i, out var paramPtr);
            if (ret != TaRetCode.Success)
            {
                TaLibException.Throw(ret, $"getting output parameter info {i} for '{name}'");
            }

            var native = Marshal.PtrToStructure<TaLibAbstractNative.TaOutputParameterInfoNative>(paramPtr);
            outputs.Add(new TaOutputParameterInfo(
                TaLibNativeLibrary.PtrToStringAnsi(native.ParamName),
                (TaOutputParameterType)native.Type,
                (TaOutputFlags)native.Flags));
        }

        return new TaFunctionDescriptor(handlePtr, functionName, group, hint, camelCaseName, (TaFunctionFlags)info.Flags, inputs, optInputs, outputs);
    }

    /// <summary>
    /// Returns the XML representation of the TA-Lib API (equivalent to <c>ta_func_api.xml</c>).
    /// </summary>
    public static string GetFunctionDescriptionXml()
    {
        TaLibCore.Initialize();
        var ptr = TaLibAbstractNative.GetFunctionDescriptionXml();
        return TaLibNativeLibrary.PtrToStringAnsi(ptr);
    }

    private static IReadOnlyList<TaValueLabel>? ReadRealList(IntPtr dataSet)
    {
        if (dataSet == IntPtr.Zero)
        {
            return null;
        }

        var list = Marshal.PtrToStructure<TaLibAbstractNative.TaRealListNative>(dataSet);
        if (list.NbElement == 0 || list.Data == IntPtr.Zero)
        {
            return Array.Empty<TaValueLabel>();
        }

        var result = new TaValueLabel[list.NbElement];
        var structSize = Marshal.SizeOf<TaLibAbstractNative.TaRealDataPairNative>();
        for (var i = 0; i < result.Length; i++)
        {
            var pairPtr = IntPtr.Add(list.Data, i * structSize);
            var pair = Marshal.PtrToStructure<TaLibAbstractNative.TaRealDataPairNative>(pairPtr);
            result[i] = new TaValueLabel(pair.Value, TaLibNativeLibrary.PtrToStringAnsi(pair.String));
        }

        return result;
    }

    private static IReadOnlyList<TaIntegerValueLabel>? ReadIntegerList(IntPtr dataSet)
    {
        if (dataSet == IntPtr.Zero)
        {
            return null;
        }

        var list = Marshal.PtrToStructure<TaLibAbstractNative.TaIntegerListNative>(dataSet);
        if (list.NbElement == 0 || list.Data == IntPtr.Zero)
        {
            return Array.Empty<TaIntegerValueLabel>();
        }

        var result = new TaIntegerValueLabel[list.NbElement];
        var pairSize = Marshal.SizeOf<TaLibAbstractNative.TaIntegerDataPairNative>();
        for (var i = 0; i < result.Length; i++)
        {
            var pairPtr = IntPtr.Add(list.Data, i * pairSize);
            var pair = Marshal.PtrToStructure<TaLibAbstractNative.TaIntegerDataPairNative>(pairPtr);
            result[i] = new TaIntegerValueLabel(pair.Value, TaLibNativeLibrary.PtrToStringAnsi(pair.String));
        }

        return result;
    }
}
