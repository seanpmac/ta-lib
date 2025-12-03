using System.Text.Json;

namespace TALib.Net.Tests;

public sealed class CandlestickParityTests
{
    [Fact]
    public void CandlestickPatterns_MatchPythonReference()
    {
        NativeDependency.Require();

        var fixture = LoadFixture();
        var patterns = new[]
        {
            "CDLADVANCEBLOCK",
            "CDLSTALLEDPATTERN",
            "CDLIDENTICAL3CROWS"
        };

        foreach (var pattern in patterns)
        {
            var series = fixture.GetProperty(pattern);
            var descriptor = TaFunctionCatalog.GetFunction(pattern);
            using var invocation = descriptor.CreateInvocation();

            var open = series.GetProperty("open").EnumerateArray().Select(x => x.GetDouble()).ToArray();
            var high = series.GetProperty("high").EnumerateArray().Select(x => x.GetDouble()).ToArray();
            var low = series.GetProperty("low").EnumerateArray().Select(x => x.GetDouble()).ToArray();
            var close = series.GetProperty("close").EnumerateArray().Select(x => x.GetDouble()).ToArray();
            var expected = series.GetProperty("output").EnumerateArray().Select(x => x.GetInt32()).ToArray();

            var rawOutput = new int[open.Length];

            invocation.SetInputPrice(0, open, high, low, close, volume: null, openInterest: null);
            invocation.SetOutputInteger(0, rawOutput);

            var result = invocation.Execute(0, open.Length - 1);
            var lookback = invocation.GetLookback();

            Assert.Equal(expected.Length, rawOutput.Length);
            Assert.Equal(lookback, result.StartIndex);
            Assert.Equal(expected.Length - lookback, result.ElementCount);

            var aligned = new int[expected.Length];
            Array.Copy(rawOutput, 0, aligned, result.StartIndex, result.ElementCount);

            for (var i = 0; i < expected.Length; i++)
            {
                Assert.Equal(expected[i], aligned[i]);
            }
        }
    }

    private static JsonElement LoadFixture()
    {
        var path = Path.Combine(AppContext.BaseDirectory, "Data", "CandlestickFixtures.json");
        if (!File.Exists(path))
        {
            path = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "Data", "CandlestickFixtures.json"));
        }

        using var stream = File.OpenRead(path);
        return JsonDocument.Parse(stream).RootElement;
    }
}
