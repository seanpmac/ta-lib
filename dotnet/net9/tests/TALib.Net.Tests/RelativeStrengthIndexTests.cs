namespace TALib.Net.Tests;

public sealed class RelativeStrengthIndexTests
{
    [Fact]
    public void CalculateRelativeStrengthIndex_ReturnsSeriesWithinBounds()
    {
        NativeDependency.Require();

        var values = new[]
        {
            44.34, 44.09, 44.15, 43.61, 44.33, 44.83, 45.10, 45.42, 45.84, 46.08,
            45.89, 46.03, 45.61, 46.28, 46.28, 46.00, 46.03, 46.41, 46.22, 45.64,
            46.21, 46.25, 45.71, 46.45, 45.78, 45.35, 44.03, 44.18, 44.22, 44.57
        };

        const int period = 14;
        var rsi = TaLibCore.CalculateRelativeStrengthIndex(values, period);

        var expectedLength = values.Length - TaLibCore.GetRelativeStrengthIndexLookback(period);
        Assert.Equal(expectedLength, rsi.Length);
        Assert.All(rsi, value => Assert.InRange(value, 0.0, 100.0));
    }

    [Fact]
    public void GetRelativeStrengthIndexLookback_ReturnsPeriod()
    {
        NativeDependency.Require();

        const int period = 14;
        Assert.Equal(period, TaLibCore.GetRelativeStrengthIndexLookback(period));
    }
}
