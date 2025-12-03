namespace TALib.Net.Tests;

public sealed class ArgumentValidationTests
{
    [Fact]
    public void CalculateSimpleMovingAverage_ThrowsWhenValuesNull()
    {
        Assert.Throws<ArgumentNullException>(() => TaLibCore.CalculateSimpleMovingAverage(null!, 3));
    }

    [Theory]
    [InlineData(0)]
    [InlineData(-1)]
    public void CalculateSimpleMovingAverage_ThrowsWhenPeriodInvalid(int period)
    {
        var values = new[] { 1.0, 2.0, 3.0 };
        Assert.Throws<ArgumentOutOfRangeException>(() => TaLibCore.CalculateSimpleMovingAverage(values, period));
    }

    [Fact]
    public void CalculateSimpleMovingAverage_ReturnsEmptyForNoValues()
    {
        var result = TaLibCore.CalculateSimpleMovingAverage(Array.Empty<double>(), 3);
        Assert.Empty(result);
    }

    [Fact]
    public void CalculateRelativeStrengthIndex_ThrowsWhenValuesNull()
    {
        Assert.Throws<ArgumentNullException>(() => TaLibCore.CalculateRelativeStrengthIndex(null!, 14));
    }

    [Theory]
    [InlineData(0)]
    [InlineData(-5)]
    public void CalculateRelativeStrengthIndex_ThrowsWhenPeriodInvalid(int period)
    {
        var values = new[] { 1.0, 2.0, 3.0 };
        Assert.Throws<ArgumentOutOfRangeException>(() => TaLibCore.CalculateRelativeStrengthIndex(values, period));
    }

    [Fact]
    public void CalculateRelativeStrengthIndex_ReturnsEmptyForNoValues()
    {
        var result = TaLibCore.CalculateRelativeStrengthIndex(Array.Empty<double>(), 14);
        Assert.Empty(result);
    }

    [Fact]
    public void CalculateMacd_ThrowsWhenValuesNull()
    {
        Assert.Throws<ArgumentNullException>(() => TaLibCore.CalculateMacd(null!, 12, 26, 9));
    }

    [Theory]
    [InlineData(0, 26, 9)]
    [InlineData(12, 0, 9)]
    [InlineData(12, 26, 0)]
    public void CalculateMacd_ThrowsWhenAnyPeriodInvalid(int fast, int slow, int signal)
    {
        var values = new[] { 1.0, 2.0, 3.0 };
        Assert.Throws<ArgumentOutOfRangeException>(() => TaLibCore.CalculateMacd(values, fast, slow, signal));
    }

    [Fact]
    public void CalculateMacd_ReturnsEmptySeriesWhenNoValues()
    {
        var macd = TaLibCore.CalculateMacd(Array.Empty<double>(), 12, 26, 9);
        Assert.Empty(macd.Macd);
        Assert.Empty(macd.Signal);
        Assert.Empty(macd.Histogram);
    }
}
