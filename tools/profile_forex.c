/* TA-Lib Profiling Tool - Real Forex Data (EURUSD)
 * Systematic performance profiling across indicator categories
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "ta_libc.h"

#define MAX_BARS 100000
#define PROFILE_RUNS 100

typedef struct {
    double *open;
    double *high;
    double *low;
    double *close;
    double *volume;
    int count;
} MarketData;

static double get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000000.0 + (double)tv.tv_usec;
}

static int load_forex_csv(const char *filepath, MarketData *data) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) return -1;

    data->open = malloc(MAX_BARS * sizeof(double));
    data->high = malloc(MAX_BARS * sizeof(double));
    data->low = malloc(MAX_BARS * sizeof(double));
    data->close = malloc(MAX_BARS * sizeof(double));
    data->volume = malloc(MAX_BARS * sizeof(double));
    data->count = 0;

    char line[512];
    while (fgets(line, sizeof(line), fp) && data->count < MAX_BARS) {
        int ms;
        double bo, bh, bl, bc, bv, ao, ah, al, ac, av;
        if (sscanf(line, "%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf",
                   &ms, &bo, &bh, &bl, &bc, &bv, &ao, &ah, &al, &ac, &av) == 11) {
            data->open[data->count] = (bo + ao) / 2.0;
            data->high[data->count] = (bh + ah) / 2.0;
            data->low[data->count] = (bl + al) / 2.0;
            data->close[data->count] = (bc + ac) / 2.0;
            data->volume[data->count] = bv + av;
            data->count++;
        }
    }
    fclose(fp);
    return data->count;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <forex_csv_file>\n", argv[0]);
        return 1;
    }

    MarketData data;
    if (load_forex_csv(argv[1], &data) < 1000) {
        fprintf(stderr, "Need at least 1000 bars\n");
        return 1;
    }

    printf("\n=== TA-Lib Performance Profiling ===\n");
    printf("Data points: %d\n", data.count);
    printf("Iterations: %d per indicator\n\n", PROFILE_RUNS);

    double *out1 = malloc(data.count * sizeof(double));
    double *out2 = malloc(data.count * sizeof(double));
    double *out3 = malloc(data.count * sizeof(double));
    int *outInt = malloc(data.count * sizeof(int));
    int outBeg, outNb;

    printf("%-25s %12s %12s %10s\n", "Indicator", "Avg (µs)", "Throughput", "Outputs");
    printf("─────────────────────────────────────────────────────────────────\n");

    // Overlap Studies
    #define PROFILE(name, call) { \
        double start = get_time_us(); \
        for(int i=0; i<PROFILE_RUNS; i++) { call; } \
        double elapsed = (get_time_us() - start) / PROFILE_RUNS; \
        double throughput = data.count / elapsed; \
        printf("%-25s %12.2f %9.0f K/s %10d\n", name, elapsed, throughput, outNb); \
    }

    PROFILE("SMA(14)", TA_SMA(0, data.count-1, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("EMA(14)", TA_EMA(0, data.count-1, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("WMA(14)", TA_WMA(0, data.count-1, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("DEMA(14)", TA_DEMA(0, data.count-1, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("TEMA(14)", TA_TEMA(0, data.count-1, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("KAMA(14)", TA_KAMA(0, data.count-1, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("MAMA(0.5,0.05)", TA_MAMA(0, data.count-1, data.close, 0.5, 0.05, &outBeg, &outNb, out1, out2));
    PROFILE("T3(5,0.7)", TA_T3(0, data.count-1, data.close, 5, 0.7, &outBeg, &outNb, out1));

    printf("\n");
    PROFILE("BBANDS(20,2,2)", TA_BBANDS(0, data.count-1, data.close, 20, 2.0, 2.0, TA_MAType_SMA, &outBeg, &outNb, out1, out2, out3));

    // Momentum Indicators
    printf("\n");
    PROFILE("RSI(14)", TA_RSI(0, data.count-1, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("MACD(12,26,9)", TA_MACD(0, data.count-1, data.close, 12, 26, 9, &outBeg, &outNb, out1, out2, out3));
    PROFILE("STOCH(14,3,3)", TA_STOCH(0, data.count-1, data.high, data.low, data.close, 14, 3, TA_MAType_SMA, 3, TA_MAType_SMA, &outBeg, &outNb, out1, out2));
    PROFILE("ADX(14)", TA_ADX(0, data.count-1, data.high, data.low, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("ADXR(14)", TA_ADXR(0, data.count-1, data.high, data.low, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("CCI(14)", TA_CCI(0, data.count-1, data.high, data.low, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("CMO(14)", TA_CMO(0, data.count-1, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("MOM(10)", TA_MOM(0, data.count-1, data.close, 10, &outBeg, &outNb, out1));
    PROFILE("ROC(10)", TA_ROC(0, data.count-1, data.close, 10, &outBeg, &outNb, out1));
    PROFILE("WILLR(14)", TA_WILLR(0, data.count-1, data.high, data.low, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("ULTOSC(7,14,28)", TA_ULTOSC(0, data.count-1, data.high, data.low, data.close, 7, 14, 28, &outBeg, &outNb, out1));

    // Volatility Indicators
    printf("\n");
    PROFILE("ATR(14)", TA_ATR(0, data.count-1, data.high, data.low, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("NATR(14)", TA_NATR(0, data.count-1, data.high, data.low, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("TRANGE", TA_TRANGE(0, data.count-1, data.high, data.low, data.close, &outBeg, &outNb, out1));
    PROFILE("STDDEV(14)", TA_STDDEV(0, data.count-1, data.close, 14, 1.0, &outBeg, &outNb, out1));
    PROFILE("VAR(14)", TA_VAR(0, data.count-1, data.close, 14, 1.0, &outBeg, &outNb, out1));

    // Hilbert Transform
    printf("\n");
    PROFILE("HT_DCPERIOD", TA_HT_DCPERIOD(0, data.count-1, data.close, &outBeg, &outNb, out1));
    PROFILE("HT_DCPHASE", TA_HT_DCPHASE(0, data.count-1, data.close, &outBeg, &outNb, out1));
    PROFILE("HT_TRENDLINE", TA_HT_TRENDLINE(0, data.count-1, data.close, &outBeg, &outNb, out1));
    PROFILE("HT_TRENDMODE", TA_HT_TRENDMODE(0, data.count-1, data.close, &outBeg, &outNb, outInt));
    PROFILE("HT_SINE", TA_HT_SINE(0, data.count-1, data.close, &outBeg, &outNb, out1, out2));
    PROFILE("HT_PHASOR", TA_HT_PHASOR(0, data.count-1, data.close, &outBeg, &outNb, out1, out2));

    // Candlestick Patterns (sample)
    printf("\n");
    PROFILE("CDLDOJI", TA_CDLDOJI(0, data.count-1, data.open, data.high, data.low, data.close, &outBeg, &outNb, outInt));
    PROFILE("CDLHAMMER", TA_CDLHAMMER(0, data.count-1, data.open, data.high, data.low, data.close, &outBeg, &outNb, outInt));
    PROFILE("CDLENGULFING", TA_CDLENGULFING(0, data.count-1, data.open, data.high, data.low, data.close, &outBeg, &outNb, outInt));
    PROFILE("CDLMORNINGSTAR", TA_CDLMORNINGSTAR(0, data.count-1, data.open, data.high, data.low, data.close, 0.3, &outBeg, &outNb, outInt));
    PROFILE("CDLEVENINGSTAR", TA_CDLEVENINGSTAR(0, data.count-1, data.open, data.high, data.low, data.close, 0.3, &outBeg, &outNb, outInt));
    PROFILE("CDL3BLACKCROWS", TA_CDL3BLACKCROWS(0, data.count-1, data.open, data.high, data.low, data.close, &outBeg, &outNb, outInt));

    // Min/Max
    printf("\n");
    PROFILE("MIN(14)", TA_MIN(0, data.count-1, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("MAX(14)", TA_MAX(0, data.count-1, data.close, 14, &outBeg, &outNb, out1));
    PROFILE("MINMAX(14)", TA_MINMAX(0, data.count-1, data.close, 14, &outBeg, &outNb, out1, out2));

    free(out1);
    free(out2);
    free(out3);
    free(outInt);
    free(data.open);
    free(data.high);
    free(data.low);
    free(data.close);
    free(data.volume);

    printf("\n");
    return 0;
}
