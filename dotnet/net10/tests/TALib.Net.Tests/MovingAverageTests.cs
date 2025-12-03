using System;
using System.Linq;

namespace TALib.Net.Tests;

public sealed class MovingAverageTests
{
    [Fact]
    public void CalculateSimpleMovingAverage_ReturnsExpectedSequence()
    {
        NativeDependency.Require();

        var values = new[] { 1.0, 2.0, 3.0, 4.0, 5.0 };
        var result = TaLibCore.CalculateSimpleMovingAverage(values, period: 3);
        Assert.Equal(values.Length, result.Length);
        Assert.True(double.IsNaN(result[0]));
        Assert.True(double.IsNaN(result[1]));
        Assert.Equal(2.0, result[2], 6);
        Assert.Equal(3.0, result[3], 6);
        Assert.Equal(4.0, result[4], 6);
    }

    [Fact]
    public void CalculateMovingAverage_RespectsRequestedType()
    {
        NativeDependency.Require();

        var values = new double[] { 1, 10, 5, 20, 3, 12, 9, 6, 15, 8, 11, 4, 13, 7, 2, 14, 16, 18, 17, 19 };
        var sma = TaLibCore.CalculateSimpleMovingAverage(values, period: 5);
        var ema = TaLibCore.CalculateMovingAverage(values, period: 5, MovingAverageType.Ema);
        Assert.Equal(values.Length, sma.Length);
        Assert.Equal(values.Length, ema.Length);
        Assert.Equal(sma.Length, ema.Length);
        Assert.NotEqual(Math.Round(sma.Last(), 6), Math.Round(ema.Last(), 6));
    }

    [Fact]
    public void GetSimpleMovingAverageLookback_ReturnsPeriodMinusOne()
    {
        NativeDependency.Require();

        var period = 7;
        var lookback = TaLibCore.GetSimpleMovingAverageLookback(period);

        Assert.Equal(period - 1, lookback);
    }
}
