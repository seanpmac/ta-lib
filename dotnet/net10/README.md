# TA-Lib .NET 10 binding

This directory hosts a modern .NET class library (`TALib.Net`) that provides a thin, P/Invoke-based wrapper over the native TA-Lib C API. The project targets .NET 10 and can be consumed from any compatible runtime (Windows, Linux, macOS). The initial surface covers moving averages (SMA, EMA, etc.), RSI, and MACD, and it can be extended by adding further DllImport entries and managed helpers.

## Prerequisites

1. Build the native TA-Lib C library for your platform (see the repository root README).
2. Ensure the resulting shared library (for example `libta_lib.so`, `libta_lib.dylib`, or `ta_lib.dll`) is discoverable at runtime. You can:
   - Place the binary in a directory already on the system loader path (`LD_LIBRARY_PATH`, `DYLD_LIBRARY_PATH`, or `PATH`).
   - Set the `TA_LIB_NATIVE_PATH` environment variable to one or more directories containing the binary (path separator delimited).
   - Call `TaLibEnvironment.AddNativeLibraryDirectory(path)` before using the wrapper to register an explicit search directory.

## Building

```bash
dotnet build TALib.Net.sln
```

The build produces `TALib.Net.dll` under `src/TALib.Net/bin/<Configuration>/net10.0/`.

### Testing

The solution also includes an integration-focused test project. Execute the suite with:

```bash
dotnet test TALib.Net.sln
```

The tests expect the native TA-Lib binary to be available. Set `TA_LIB_NATIVE_PATH` (or call `TaLibEnvironment.AddNativeLibraryDirectory`) before running them so the interop layer can resolve the shared library.

### Calling any indicator via TA-Abstract

In addition to the convenience helpers in `TaLibCore`, the binding exposes the full TA-Abstract interface so you can discover metadata and invoke every indicator dynamically:

```csharp
using TALib.Net;

TaLibEnvironment.AddNativeLibraryDirectory("/path/to/native/ta-lib");

var rsi = TaFunctionCatalog.GetFunction("RSI");
using var call = rsi.CreateInvocation();

var prices = GetPriceSeries();
call.SetInputReal(0, prices);
call.SetOptionalInteger(0, 14);          // optInTimePeriod

var output = new double[prices.Length];
call.SetOutputReal(0, output);

var lookback = call.GetLookback();
var result = call.Execute(0, prices.Length - 1);

Console.WriteLine($"Outputs from index {result.StartIndex} ({result.ElementCount} samples)");
```

## Using the wrapper

```csharp
using TALib.Net;

TaLibEnvironment.AddNativeLibraryDirectory("/path/to/native/ta-lib");
TaLibCore.Initialize();

var prices = new double[] { 44.22, 44.45, 44.21, 43.96, 44.05 };
var sma = TaLibCore.CalculateSimpleMovingAverage(prices, period: 3);

Console.WriteLine(string.Join(", ", sma));

TaLibCore.Shutdown();
```

`TaLibCore.Initialize()` is idempotent; it is called automatically when invoking calculation helpers if the library is not yet ready. The helpers surface `TaLibException` when the native layer returns an error code, exposing the corresponding `TaRetCode`.

Refer to the source under `src/TALib.Net` for further details and extend the wrapper with additional indicators as needed.
