using System.Diagnostics.CodeAnalysis;

namespace TALib.Net;

/// <summary>
/// Exception type thrown when the native TA-Lib library reports an error.
/// </summary>
public sealed class TaLibException : Exception
{
    /// <summary>
    /// Initializes a new instance of the <see cref="TaLibException"/> class.
    /// </summary>
    /// <param name="retCode">The return code reported by TA-Lib.</param>
    /// <param name="message">Optional human-readable description.</param>
    public TaLibException(TaRetCode retCode, string? message = null)
        : base(message ?? BuildMessage(retCode))
    {
        RetCode = retCode;
    }

    /// <summary>
    /// Gets the return code reported by TA-Lib.
    /// </summary>
    public TaRetCode RetCode { get; }

    [DoesNotReturn]
    internal static void Throw(TaRetCode retCode, string context)
    {
        throw new TaLibException(retCode, $"TA-Lib error ({retCode}) while {context}.");
    }

    private static string BuildMessage(TaRetCode retCode) => $"TA-Lib returned {retCode}.";
}
