using System;
using System.IO;
using System.Threading;
using Xunit;
using Xunit.Sdk;

namespace TALib.Net.Tests;

internal static class NativeDependency
{
    private static readonly Lazy<bool> IsAvailable = new(() =>
    {
        TryDiscoverNativeLibrary();

        try
        {
            TaLibCore.Initialize();
            return true;
        }
        catch (DllNotFoundException)
        {
            return false;
        }
        catch (TaLibException)
        {
            return false;
        }
    }, LazyThreadSafetyMode.ExecutionAndPublication);

    internal static bool EnsureAvailable() => IsAvailable.Value;

    internal static void Require()
    {
        if (!EnsureAvailable())
        {
            var searchPaths = string.Join(", ", TaLibEnvironment.GetNativeLibraryDirectories());
            throw new Xunit.Sdk.XunitException($"Native TA-Lib library not found. Configure TA_LIB_NATIVE_PATH or call TaLibEnvironment.AddNativeLibraryDirectory(). Probed directories: {searchPaths}");
        }
    }

    private static void TryDiscoverNativeLibrary()
    {
        if (Environment.GetEnvironmentVariable("TA_LIB_NATIVE_PATH") is { Length: > 0 })
        {
            return;
        }

        var baseDir = AppContext.BaseDirectory;
        for (var i = 0; i < 8; i++)
        {
            var candidateRoot = Path.GetFullPath(baseDir);
            if (TryRegisterFromRoot(candidateRoot))
            {
                return;
            }

            baseDir = Path.GetFullPath(Path.Combine(baseDir, ".."));
        }
    }

    private static bool TryRegisterFromRoot(string root)
    {
        var searchCandidates = new[]
        {
            root,
            Path.Combine(root, "build"),
            Path.Combine(root, "build", "lib"),
            Path.Combine(root, "lib"),
        };

        foreach (var candidate in searchCandidates)
        {
            if (!Directory.Exists(candidate))
            {
                continue;
            }

            if (Directory.GetFiles(candidate, "libta*-*.dylib").Length > 0 ||
                Directory.GetFiles(candidate, "libta-*.so").Length > 0 ||
                Directory.GetFiles(candidate, "ta-*.dll").Length > 0 ||
                Directory.GetFiles(candidate, "libta_lib*.dylib").Length > 0 ||
                Directory.GetFiles(candidate, "libta_lib*.so").Length > 0 ||
                Directory.GetFiles(candidate, "ta_lib*.dll").Length > 0)
            {
                TaLibEnvironment.AddNativeLibraryDirectory(candidate);
                return true;
            }
        }

        return false;
    }
}
