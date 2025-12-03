using System.Text.Json;

namespace TALib.Net.Tests;

public sealed class PythonParityTests
{
    private const double Tolerance = 1e-9;

    [Fact]
    public void SimpleMovingAverage_MatchesPython()
    {
        NativeDependency.Require();

        var fixture = LoadFixture();
        var values = new[] { 1.0, 2.0, 3.0, 4.0, 5.0 };

        var result = TaLibCore.CalculateSimpleMovingAverage(values, 3);
        AssertEqualSeries(fixture.GetProperty("sma"), result);
    }

    [Fact]
    public void RelativeStrengthIndex_MatchesPython()
    {
        NativeDependency.Require();

        var fixture = LoadFixture();
        var values = new[]
        {
            44.34, 44.09, 44.15, 43.61, 44.33, 44.83, 45.10, 45.42, 45.84, 46.08,
            45.89, 46.03, 45.61, 46.28, 46.28, 46.00, 46.03, 46.41, 46.22, 45.64,
            46.21, 46.25, 45.71, 46.45, 45.78, 45.35, 44.03, 44.18, 44.22, 44.57
        };

        var result = TaLibCore.CalculateRelativeStrengthIndex(values, 14);
        AssertEqualSeries(fixture.GetProperty("rsi"), result);
    }

    [Fact]
    public void Macd_MatchesPython()
    {
        NativeDependency.Require();

        var fixture = LoadFixture().GetProperty("macd");
        var values = new[]
        {
            81.59, 81.06, 82.87, 83.00, 83.61, 83.15, 82.84, 83.99, 84.55, 84.36,
            85.53, 86.54, 86.89, 87.77, 87.29, 86.96, 87.07, 87.29, 86.64, 86.63,
            86.21, 87.70, 87.85, 87.67, 86.43, 83.16, 83.46, 83.81, 83.23, 82.45,
            82.87, 83.23, 84.03, 84.33, 84.57, 84.12, 85.81, 86.03, 87.10, 87.47
        };

        var result = TaLibCore.CalculateMacd(values, 12, 26, 9);
        AssertEqualSeries(fixture.GetProperty("macd"), result.Macd);
        AssertEqualSeries(fixture.GetProperty("signal"), result.Signal);
        AssertEqualSeries(fixture.GetProperty("hist"), result.Histogram);
    }

    private static JsonElement LoadFixture()
    {
        var path = Path.Combine(AppContext.BaseDirectory, "Data", "PythonFixtures.json");
        if (!File.Exists(path))
        {
            // Fallback to source-relative location when running from IDEs or custom runners.
            path = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "Data", "PythonFixtures.json"));
        }

        using var stream = File.OpenRead(path);
        return JsonDocument.Parse(stream).RootElement;
    }

    private static void AssertEqualSeries(JsonElement expected, IReadOnlyList<double> actual)
    {
        Assert.Equal(expected.GetArrayLength(), actual.Count);

        for (var i = 0; i < actual.Count; i++)
        {
            if (expected[i].ValueKind == JsonValueKind.Null)
            {
                Assert.True(double.IsNaN(actual[i]), $"Expected NaN at index {i}, got {actual[i]}");
            }
            else
            {
                var expectedValue = expected[i].GetDouble();
                Assert.InRange(actual[i], expectedValue - Tolerance, expectedValue + Tolerance);
            }
        }
    }
}
