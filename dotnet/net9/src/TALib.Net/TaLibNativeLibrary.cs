using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

namespace TALib.Net;

internal static class TaLibNativeLibrary
{
    internal const string LibraryName = "ta_lib";

    private static readonly object SyncRoot = new();
    private static readonly List<string> SearchPaths = new();
    private static readonly HashSet<string> SearchPathSet =
        new(RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
            ? StringComparer.OrdinalIgnoreCase
            : StringComparer.Ordinal);

    private static bool _resolverRegistered;
    private static bool _environmentPathsLoaded;

    internal static void EnsureResolver()
    {
        lock (SyncRoot)
        {
            if (_resolverRegistered)
            {
                return;
            }

            LoadEnvironmentHintPathsNoLock();
            NativeLibrary.SetDllImportResolver(typeof(TaLibNativeLibrary).Assembly, ResolveLibrary);
            _resolverRegistered = true;
        }
    }

    internal static void AddSearchPath(string path)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            throw new ArgumentException("Search path cannot be null or whitespace.", nameof(path));
        }

        lock (SyncRoot)
        {
            LoadEnvironmentHintPathsNoLock();
            AddSearchPathInternal(path);
        }
    }

    internal static IReadOnlyList<string> GetSearchPaths()
    {
        lock (SyncRoot)
        {
            LoadEnvironmentHintPathsNoLock();
            return SearchPaths.ToArray();
        }
    }

    internal static string PtrToStringAnsi(IntPtr pointer)
    {
        if (pointer == IntPtr.Zero)
        {
            return string.Empty;
        }

        return Marshal.PtrToStringAnsi(pointer) ?? string.Empty;
    }

    private static IntPtr ResolveLibrary(string libraryName, Assembly assembly, DllImportSearchPath? searchPath)
    {
        if (!string.Equals(libraryName, LibraryName, StringComparison.Ordinal))
        {
            return IntPtr.Zero;
        }

        lock (SyncRoot)
        {
            LoadEnvironmentHintPathsNoLock();

            var candidateNames = GetCandidateLibraryNames().ToArray();
            var candidateDirectories = EnumerateCandidateDirectories().ToArray();

            if (TryLoad(assembly, searchPath, libraryName, out var handle))
            {
                return handle;
            }

            foreach (var candidate in candidateNames)
            {
                if (TryLoad(assembly, searchPath, candidate, out handle))
                {
                    return handle;
                }
            }

            foreach (var directory in candidateDirectories)
            {
                foreach (var candidate in candidateNames)
                {
                    var fullPath = Path.Combine(directory, candidate);
                    if (NativeLibrary.TryLoad(fullPath, out handle))
                    {
                        return handle;
                    }
                }
            }

            throw new DllNotFoundException(BuildErrorMessage(candidateNames, candidateDirectories));
        }
    }

    private static bool TryLoad(Assembly assembly, DllImportSearchPath? searchPath, string name, out IntPtr handle)
    {
        return NativeLibrary.TryLoad(name, assembly, searchPath, out handle) ||
               NativeLibrary.TryLoad(name, out handle);
    }

    private static string BuildErrorMessage(IEnumerable<string> candidateNames, IEnumerable<string> directories)
    {
        var builder = new StringBuilder();
        builder.AppendLine("Unable to locate the native TA-Lib library.");
        builder.Append("Probed file names: ");
        builder.AppendLine(string.Join(", ", candidateNames));
        builder.Append("Search directories: ");
        builder.AppendLine(string.Join(", ", directories));
        builder.AppendLine("Set the TA_LIB_NATIVE_PATH environment variable or call TaLibEnvironment.AddNativeLibraryDirectory(path) before using the library.");
        return builder.ToString();
    }

    private static IEnumerable<string> GetCandidateLibraryNames()
    {
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            yield return LibraryName;
            yield return "ta_lib.dll";
            yield return "ta_libc.dll";
            yield return "ta-lib.dll";
            yield return "ta-libc.dll";
        }
        else if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
        {
            yield return LibraryName;
            yield return "libta_lib.dylib";
            yield return "libta_lib.0.dylib";
            yield return "ta_lib.dylib";
            yield return "libta-lib.dylib";
            yield return "libta-lib.0.dylib";
            yield return "ta-lib.dylib";
        }
        else
        {
            yield return LibraryName;
            yield return "libta_lib.so";
            yield return "libta_lib.so.0";
            yield return "ta_lib.so";
            yield return "libta-lib.so";
            yield return "libta-lib.so.0";
            yield return "ta-lib.so";
        }
    }

    private static IEnumerable<string> EnumerateCandidateDirectories()
    {
        foreach (var candidate in EnumerateRawCandidateDirectories())
        {
            if (string.IsNullOrWhiteSpace(candidate))
            {
                continue;
            }

            string fullPath;
            try
            {
                fullPath = Path.GetFullPath(candidate);
            }
            catch (Exception)
            {
                continue;
            }

            if (!Directory.Exists(fullPath))
            {
                continue;
            }

            yield return fullPath;
        }
    }

    private static IEnumerable<string> EnumerateRawCandidateDirectories()
    {
        var baseDirectory = AppContext.BaseDirectory;
        var currentDirectory = Environment.CurrentDirectory;
        var runtimeIdentifier = RuntimeInformation.RuntimeIdentifier;

        yield return baseDirectory;
        yield return Path.Combine(baseDirectory, "native");
        yield return Path.Combine(baseDirectory, "runtimes", runtimeIdentifier, "native");
        yield return currentDirectory;
        yield return Path.Combine(currentDirectory, "lib");
        yield return Path.Combine(currentDirectory, "build", "lib");
        yield return Path.Combine(baseDirectory, "..", "lib");
        yield return Path.Combine(baseDirectory, "..", "build", "lib");

        foreach (var path in SearchPaths)
        {
            yield return path;
        }
    }

    private static void LoadEnvironmentHintPathsNoLock()
    {
        if (_environmentPathsLoaded)
        {
            return;
        }

        _environmentPathsLoaded = true;

        var configuredPaths = Environment.GetEnvironmentVariable("TA_LIB_NATIVE_PATH");
        if (!string.IsNullOrWhiteSpace(configuredPaths))
        {
            foreach (var segment in configuredPaths.Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries))
            {
                AddSearchPathInternal(segment);
            }
        }

        var taLibHome = Environment.GetEnvironmentVariable("TA_LIB_HOME");
        if (!string.IsNullOrWhiteSpace(taLibHome))
        {
            AddSearchPathInternal(taLibHome);
            AddSearchPathInternal(Path.Combine(taLibHome, "lib"));
            AddSearchPathInternal(Path.Combine(taLibHome, "build", "lib"));
        }
    }

    private static void AddSearchPathInternal(string path)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            return;
        }

        string normalizedPath;
        try
        {
            normalizedPath = Path.GetFullPath(path);
        }
        catch (Exception)
        {
            return;
        }

        if (!SearchPathSet.Add(normalizedPath))
        {
            return;
        }

        SearchPaths.Add(normalizedPath);
    }
}
