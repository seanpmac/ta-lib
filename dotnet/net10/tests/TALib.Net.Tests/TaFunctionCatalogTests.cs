using System;
using Xunit;

namespace TALib.Net.Tests;

public sealed class TaFunctionCatalogTests
{
    [Fact]
    public void GetFunction_ReturnsMetadataForKnownIndicator()
    {
        NativeDependency.Require();

        var descriptor = TaFunctionCatalog.GetFunction("RSI");
        Assert.Equal("RSI", descriptor.Name);
        Assert.NotEmpty(descriptor.Group);
        Assert.Single(descriptor.Inputs);
        Assert.True(descriptor.OptionalInputs.Count >= 1);
        Assert.Single(descriptor.Outputs);
    }

    [Fact]
    public void Invocation_ComputesRsiViaAbstractInterface()
    {
        NativeDependency.Require();

        var descriptor = TaFunctionCatalog.GetFunction("RSI");
        using var invocation = descriptor.CreateInvocation();

        var values = new[]
        {
            44.34, 44.09, 44.15, 43.61, 44.33, 44.83, 45.10, 45.42, 45.84, 46.08,
            45.89, 46.03, 45.61, 46.28, 46.28, 46.00, 46.03, 46.41, 46.22, 45.64,
            46.21, 46.25, 45.71, 46.45, 45.78, 45.35, 44.03, 44.18, 44.22, 44.57
        };

        invocation.SetInputReal(0, values);

        var periodOptIndex = -1;
        for (var i = 0; i < descriptor.OptionalInputs.Count; i++)
        {
            if (descriptor.OptionalInputs[i].Name.Equals("optInTimePeriod", StringComparison.OrdinalIgnoreCase))
            {
                periodOptIndex = i;
                break;
            }
        }

        if (periodOptIndex >= 0)
        {
            invocation.SetOptionalInteger(periodOptIndex, 14);
        }

        var output = new double[values.Length];
        invocation.SetOutputReal(0, output);

        var lookback = invocation.GetLookback();
        var result = invocation.Execute(0, values.Length - 1);

        Assert.Equal(lookback, result.StartIndex);
        Assert.Equal(values.Length - lookback, result.ElementCount);
        Assert.InRange(output[result.StartIndex], 0.0, 100.0);
    }
}
