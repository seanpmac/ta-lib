namespace TALib.Net;

/// <summary>
/// Provides configuration helpers for locating the native TA-Lib binaries.
/// </summary>
public static class TaLibEnvironment
{
    /// <summary>
    /// Adds a directory to the search path used when resolving the native TA-Lib shared library.
    /// </summary>
    /// <param name="path">Absolute or relative path containing the TA-Lib binaries.</param>
    /// <exception cref="ArgumentException">Thrown when <paramref name="path"/> is null or whitespace.</exception>
    public static void AddNativeLibraryDirectory(string path) => TaLibNativeLibrary.AddSearchPath(path);

    /// <summary>
    /// Gets the list of directories that will be probed for the native TA-Lib shared library.
    /// </summary>
    public static IReadOnlyList<string> GetNativeLibraryDirectories() => TaLibNativeLibrary.GetSearchPaths();
}
