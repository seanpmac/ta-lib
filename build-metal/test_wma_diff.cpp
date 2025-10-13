#include <vector>
#include <cstdio>
#include <cmath>

static void fill_series(std::vector<double> &buf)
{
    double scale = 100.0;
    size_t length = buf.size();
    for (size_t i = 0; i < length; ++i) {
        double t = (double)i / (double)length;
        buf[i] = scale * (sin(20.0 * t) + 0.5 * cos(3.0 * t)) + 100.0;
    }
}

int main()
{
    const size_t length = 2000000;
    const int period = 30;
    const size_t outputCount = length - period + 1;

    std::vector<double> data(length);
    fill_series(data);

    std::vector<double> prefix(length);
    double run = 0.0;
    for (size_t i = 0; i < length; ++i) {
        run += data[i];
        prefix[i] = run;
    }

    std::vector<double> weighted(length);
    std::vector<double> prefixWeighted(length);
    run = 0.0;
    for (size_t i = 0; i < length; ++i) {
        weighted[i] = data[i] * (double)(i + 1);
        run += weighted[i];
        prefixWeighted[i] = run;
    }

    std::vector<double> cpu(outputCount);
    double totalWeight = (double)period * (double)(period + 1) / 2.0;
    for (size_t gid = 0; gid < outputCount; ++gid) {
        size_t endIndex = gid + (size_t)period - 1;
        double endValue = prefix[endIndex];
        double startValue = (gid == 0) ? 0.0 : prefix[gid - 1];
        double weightedEnd = prefixWeighted[endIndex];
        double weightedStart = (gid == 0) ? 0.0 : prefixWeighted[gid - 1];
        double baseSum = endValue - startValue;
        double numerator = (weightedEnd - weightedStart) - (double)gid * baseSum;
        cpu[gid] = numerator / totalWeight;
    }

    std::vector<float> prefixF(length);
    std::vector<float> prefixWeightedF(length);
    for (size_t i = 0; i < length; ++i) {
        prefixF[i] = (float)prefix[i];
        prefixWeightedF[i] = (float)prefixWeighted[i];
    }

    std::vector<float> gpuLike(outputCount);
    float totalWeightF = (float)period * (float)(period + 1) * 0.5f;
    for (size_t gid = 0; gid < outputCount; ++gid) {
        size_t endIndex = gid + (size_t)period - 1;
        float endValue = prefixF[endIndex];
        float startValue = (gid == 0) ? 0.0f : prefixF[gid - 1];
        float weightedEnd = prefixWeightedF[endIndex];
        float weightedStart = (gid == 0) ? 0.0f : prefixWeightedF[gid - 1];
        float baseSum = endValue - startValue;
        float numerator = (weightedEnd - weightedStart) - (float)gid * baseSum;
        gpuLike[gid] = numerator / totalWeightF;
    }

    double maxDiff = 0.0;
    size_t maxIdx = 0;
    for (size_t i = 0; i < outputCount; ++i) {
        double diff = fabs((double)gpuLike[i] - cpu[i]);
        if (diff > maxDiff) {
            maxDiff = diff;
            maxIdx = i;
        }
    }

    printf("max diff = %.6f at index %zu (cpu=%.6f gpuLike=%.6f)\n", maxDiff, maxIdx, cpu[maxIdx], gpuLike[maxIdx]);
    return 0;
}
