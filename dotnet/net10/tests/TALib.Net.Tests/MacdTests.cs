using System.Linq;

namespace TALib.Net.Tests;

public sealed class MacdTests
{
    [Fact]
    public void CalculateMacd_ReturnsAlignedSeries()
    {
        NativeDependency.Require();

        var values = new[]
        {
            81.59, 81.06, 82.87, 83.00, 83.61, 83.15, 82.84, 83.99, 84.55, 84.36,
            85.53, 86.54, 86.89, 87.77, 87.29, 86.96, 87.07, 87.29, 86.64, 86.63,
            86.21, 87.70, 87.85, 87.67, 86.43, 83.16, 83.46, 83.81, 83.23, 82.45,
            82.87, 83.23, 84.03, 84.33, 84.57, 84.12, 85.81, 86.03, 87.10, 87.47
        };

        const int fast = 12;
        const int slow = 26;
        const int signal = 9;

        var macd = TaLibCore.CalculateMacd(values, fast, slow, signal);
        var lookback = TaLibCore.GetMacdLookback(fast, slow, signal);

        Assert.Equal(values.Length, macd.Macd.Length);
        Assert.Equal(values.Length, macd.Signal.Length);
        Assert.Equal(values.Length, macd.Histogram.Length);

        Assert.All(macd.Macd.Take(lookback), value => Assert.True(double.IsNaN(value)));
        Assert.All(macd.Signal.Take(lookback), value => Assert.True(double.IsNaN(value)));
        Assert.All(macd.Histogram.Take(lookback), value => Assert.True(double.IsNaN(value)));

        for (var i = lookback; i < macd.Macd.Length; i++)
        {
            var diff = macd.Macd[i] - macd.Signal[i];
            Assert.Equal(diff, macd.Histogram[i], 6);
        }
    }

    [Fact]
    public void GetMacdLookback_FollowsExpectedFormula()
    {
        NativeDependency.Require();

        const int fast = 5;
        const int slow = 21;
        const int signal = 9;

        var expected = (Math.Max(fast, slow) - 1) + (signal - 1);
        Assert.Equal(expected, TaLibCore.GetMacdLookback(fast, slow, signal));
    }
}
