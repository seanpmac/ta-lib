/* Indicator runners - Serial and parallel execution with timing */

#include "ta_perf.h"
#include "ta_libc.h"
#include "ta_abstract.h"
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_OPENMP
#include <omp.h>
#endif

/* Complete list of all 165 TA-Lib indicators */
static const char* ALL_INDICATORS[] = {
    /* Overlap Studies */
    "ACCBANDS", "BBANDS", "DEMA", "EMA", "HT_TRENDLINE", "KAMA", "MA", "MAMA",
    "MAVP", "MIDPOINT", "MIDPRICE", "SAR", "SAREXT", "SMA", "T3", "TEMA",
    "TRIMA", "WMA",
    
    /* Momentum Indicators */
    "ADX", "ADXR", "APO", "AROON", "AROONOSC", "BOP", "CCI", "CMO", "DX",
    "IMI", "MACD", "MACDEXT", "MACDFIX", "MFI", "MINUS_DI", "MINUS_DM",
    "MOM", "PLUS_DI", "PLUS_DM", "PPO", "ROC", "ROCP", "ROCR", "ROCR100",
    "RSI", "STOCH", "STOCHF", "STOCHRSI", "TRIX", "ULTOSC", "WILLR",
    
    /* Volume Indicators */
    "AD", "ADOSC", "OBV", "NVI", "PVI",
    
    /* Volatility Indicators */
    "ATR", "NATR", "TRANGE",
    
    /* Price Transform */
    "AVGPRICE", "MEDPRICE", "TYPPRICE", "WCLPRICE",
    
    /* Cycle Indicators */
    "HT_DCPERIOD", "HT_DCPHASE", "HT_PHASOR", "HT_SINE", "HT_TRENDMODE",
    
    /* Pattern Recognition (Candlesticks) */
    "CDL2CROWS", "CDL3BLACKCROWS", "CDL3INSIDE", "CDL3LINESTRIKE", "CDL3OUTSIDE",
    "CDL3STARSINSOUTH", "CDL3WHITESOLDIERS", "CDLABANDONEDBABY", "CDLADVANCEBLOCK",
    "CDLBELTHOLD", "CDLBREAKAWAY", "CDLCLOSINGMARUBOZU", "CDLCONCEALBABYSWALL",
    "CDLCOUNTERATTACK", "CDLDARKCLOUDCOVER", "CDLDOJI", "CDLDOJISTAR",
    "CDLDRAGONFLYDOJI", "CDLENGULFING", "CDLEVENINGDOJISTAR", "CDLEVENINGSTAR",
    "CDLGAPSIDESIDEWHITE", "CDLGRAVESTONEDOJI", "CDLHAMMER", "CDLHANGINGMAN",
    "CDLHARAMI", "CDLHARAMICROSS", "CDLHIGHWAVE", "CDLHIKKAKE", "CDLHIKKAKEMOD",
    "CDLHOMINGPIGEON", "CDLIDENTICAL3CROWS", "CDLINNECK", "CDLINVERTEDHAMMER",
    "CDLKICKING", "CDLKICKINGBYLENGTH", "CDLLADDERBOTTOM", "CDLLONGLEGGEDDOJI",
    "CDLLONGLINE", "CDLMARUBOZU", "CDLMATCHINGLOW", "CDLMATHOLD",
    "CDLMORNINGDOJISTAR", "CDLMORNINGSTAR", "CDLONNECK", "CDLPIERCING",
    "CDLRICKSHAWMAN", "CDLRISEFALL3METHODS", "CDLSEPARATINGLINES", "CDLSHOOTINGSTAR",
    "CDLSHORTLINE", "CDLSPINNINGTOP", "CDLSTALLEDPATTERN", "CDLSTICKSANDWICH",
    "CDLTAKURI", "CDLTASUKIGAP", "CDLTHRUSTING", "CDLTRISTAR", "CDLUNIQUE3RIVER",
    "CDLUPSIDEGAP2CROWS", "CDLXSIDEGAP3METHODS",
    
    /* Statistic Functions */
    "BETA", "CORREL", "LINEARREG", "LINEARREG_ANGLE", "LINEARREG_INTERCEPT",
    "LINEARREG_SLOPE", "STDDEV", "TSF", "VAR",
    
    /* Math Transform */
    "ACOS", "ASIN", "ATAN", "CEIL", "COS", "COSH", "EXP", "FLOOR", "LN",
    "LOG10", "SIN", "SINH", "SQRT", "TAN", "TANH",
    
    /* Math Operators */
    "ADD", "DIV", "MAX", "MAXINDEX", "MIN", "MININDEX", "MINMAX", "MINMAXINDEX",
    "MULT", "SUB", "SUM",
    
    /* Additional */
    "ALLIGATOR", "AVGDEV",
    
    NULL
};

/* Helper: check if indicator is in filter list */
static int should_run_indicator(const char *name, const char *filter) {
    if (filter[0] == '\0') return 1;  /* No filter = run all */
    
    char filter_copy[512];
    strncpy(filter_copy, filter, 511);
    filter_copy[511] = '\0';
    
    char *token = strtok(filter_copy, ",");
    while (token) {
        if (strcmp(token, name) == 0) return 1;
        token = strtok(NULL, ",");
    }
    return 0;
}

/* Helper: store indicator result with capacity guard */
static int record_indicator_result(RunResult *result, const IndicatorResult *ind_result) {
    static int warned = 0;

    if (result->indicator_count >= MAX_INDICATORS) {
        if (!warned) {
            fprintf(stderr,
                "ta_perf warning: indicator buffer capacity (%d) exceeded; additional results will be dropped.\n",
                MAX_INDICATORS);
            warned = 1;
        }
        return -1;
    }

    result->indicators[result->indicator_count++] = *ind_result;
    return 0;
}

/* Helper: call indicator and measure time using TA-Lib abstract interface */
static int run_indicator(const char *name, const PriceData *data, IndicatorResult *result) {
    TA_RetCode retCode;
    const TA_FuncHandle *handle;
    const TA_FuncInfo *info;
    TA_ParamHolder *params;
    int outBegIdx, outNbElement;
    
    /* Get function handle */
    retCode = TA_GetFuncHandle(name, &handle);
    if (retCode != TA_SUCCESS) {
        return 1;  /* Skip - function not found */
    }
    
    retCode = TA_GetFuncInfo(handle, &info);
    if (retCode != TA_SUCCESS) {
        return 1;
    }
    
    /* Allocate parameter holder */
    retCode = TA_ParamHolderAlloc(handle, &params);
    if (retCode != TA_SUCCESS) {
        return -1;
    }
    
    /* Set input parameters based on function requirements */
    const TA_InputParameterInfo *inputInfo;
    for (int i = 0; i < info->nbInput; i++) {
        retCode = TA_GetInputParameterInfo(handle, i, &inputInfo);
        if (retCode != TA_SUCCESS) {
            TA_ParamHolderFree(params);
            return -1;
        }
        
        switch (inputInfo->type) {
            case TA_Input_Price:
                /* Price inputs need OHLCV data - use special price setter */
                retCode = TA_SetInputParamPricePtr(params, i,
                    data->open, data->high, data->low, data->close,
                    data->volume, NULL);  /* No open interest */
                if (retCode != TA_SUCCESS) {
                    TA_ParamHolderFree(params);
                    return -1;
                }
                break;
                
            case TA_Input_Real:
                /* Single real input - use close price */
                retCode = TA_SetInputParamRealPtr(params, i, data->close);
                if (retCode != TA_SUCCESS) {
                    TA_ParamHolderFree(params);
                    return -1;
                }
                break;
                
            case TA_Input_Integer:
                /* Integer input - rarely used, skip for now */
                break;
        }
    }
    
    /* Set optional parameters to defaults */
    const TA_OptInputParameterInfo *optInfo;
    for (int i = 0; i < info->nbOptInput; i++) {
        retCode = TA_GetOptInputParameterInfo(handle, i, &optInfo);
        if (retCode != TA_SUCCESS) {
            TA_ParamHolderFree(params);
            return -1;
        }
        
        /* Use default value */
        switch (optInfo->type) {
            case TA_OptInput_RealRange:
            case TA_OptInput_RealList:
                TA_SetOptInputParamReal(params, i, optInfo->defaultValue);
                break;
            case TA_OptInput_IntegerRange:
            case TA_OptInput_IntegerList:
                TA_SetOptInputParamInteger(params, i, (int)optInfo->defaultValue);
                break;
        }
    }
    
    /* Allocate output buffers */
    double *outReal0 = malloc(data->bar_count * sizeof(double));
    double *outReal1 = malloc(data->bar_count * sizeof(double));
    double *outReal2 = malloc(data->bar_count * sizeof(double));
    int *outInt = malloc(data->bar_count * sizeof(int));
    
    if (!outReal0 || !outReal1 || !outReal2 || !outInt) {
        free(outReal0);
        free(outReal1);
        free(outReal2);
        free(outInt);
        TA_ParamHolderFree(params);
        return -1;
    }
    
    /* Set output parameters */
    const TA_OutputParameterInfo *outInfo;
    for (int i = 0; i < info->nbOutput; i++) {
        retCode = TA_GetOutputParameterInfo(handle, i, &outInfo);
        if (retCode != TA_SUCCESS) {
            free(outReal0);
            free(outReal1);
            free(outReal2);
            free(outInt);
            TA_ParamHolderFree(params);
            return -1;
        }
        
        if (outInfo->type == TA_Output_Real) {
            if (i == 0) TA_SetOutputParamRealPtr(params, i, outReal0);
            else if (i == 1) TA_SetOutputParamRealPtr(params, i, outReal1);
            else if (i == 2) TA_SetOutputParamRealPtr(params, i, outReal2);
        } else if (outInfo->type == TA_Output_Integer) {
            TA_SetOutputParamIntegerPtr(params, i, outInt);
        }
    }
    
    /* Execute indicator with timing */
    uint64_t start = timer_now_us();
    retCode = TA_CallFunc(params, 0, data->bar_count - 1, &outBegIdx, &outNbElement);
    uint64_t end = timer_now_us();
    
    /* Cleanup */
    free(outReal0);
    free(outReal1);
    free(outReal2);
    free(outInt);
    TA_ParamHolderFree(params);
    
    if (retCode != TA_SUCCESS) {
        return -1;
    }
    
    /* Record timing */
    strncpy(result->name, name, 63);
    result->name[63] = '\0';
    result->calls = 1;
    double elapsed = timer_elapsed_us(start, end);
    result->total_time_us = elapsed;
    result->min_time_us = elapsed;
    result->max_time_us = elapsed;
    result->avg_time_us = elapsed;
    result->bars_processed = data->bar_count;
    
    return 0;
}

int run_serial(const PriceData *data, const Config *config, RunResult *result) {
    result->mode = MODE_SERIAL;
    result->thread_count = 1;
    result->indicator_count = 0;
    
    uint64_t total_start = timer_now_us();
    
    for (int i = 0; ALL_INDICATORS[i] != NULL; i++) {
        const char *name = ALL_INDICATORS[i];
        
        if (!should_run_indicator(name, config->indicator_filter)) {
            continue;
        }
        
        IndicatorResult ind_result = {0};
        int ret = run_indicator(name, data, &ind_result);
        
        if (ret == 0) {
            if (record_indicator_result(result, &ind_result) != 0) {
                /* Buffer full; stop early to avoid further work */
                break;
            }
        } else if (ret < 0) {
            fprintf(stderr, "Error running %s\n", name);
            return -1;
        }
        /* ret == 1 means skip (not implemented) */
    }
    
    uint64_t total_end = timer_now_us();
    result->total_time_us = timer_elapsed_us(total_start, total_end);
    
    return 0;
}

int run_parallel(const PriceData *data, const Config *config, RunResult *result) {
    result->mode = MODE_PARALLEL;
    
    /* Determine thread count */
    int thread_count = config->thread_count;
    if (thread_count == 0) {
#ifdef HAVE_OPENMP
        thread_count = omp_get_max_threads();
#else
        thread_count = 1;  /* Fallback to serial */
#endif
    }
    if (thread_count > config->max_threads) {
        thread_count = config->max_threads;
    }
    result->thread_count = thread_count;
    
    /* Count indicators to run */
    int indicator_count = 0;
    for (int i = 0; ALL_INDICATORS[i] != NULL; i++) {
        if (should_run_indicator(ALL_INDICATORS[i], config->indicator_filter)) {
            indicator_count++;
        }
    }
    
    result->indicator_count = 0;
    uint64_t total_start = timer_now_us();
    
    // Count indicators first (OpenMP requires known iteration count)
    int num_indicators = 0;
    for (int i = 0; ALL_INDICATORS[i] != NULL; i++) {
        num_indicators++;
    }
    
#ifdef HAVE_OPENMP
    #pragma omp parallel num_threads(thread_count)
    {
        #pragma omp for schedule(dynamic)
        for (int i = 0; i < num_indicators; i++) {
            const char *name = ALL_INDICATORS[i];
            
            if (!should_run_indicator(name, config->indicator_filter)) {
                continue;
            }
            
            IndicatorResult ind_result = {0};
            int ret = run_indicator(name, data, &ind_result);
            
            if (ret == 0) {
                #pragma omp critical
                {
                    record_indicator_result(result, &ind_result);
                }
            } else if (ret < 0) {
                #pragma omp critical
                {
                    fprintf(stderr, "Error running %s\n", name);
                }
            }
        }
    }
#else
    /* Fallback to serial if OpenMP not available */
    return run_serial(data, config, result);
#endif
    
    uint64_t total_end = timer_now_us();
    result->total_time_us = timer_elapsed_us(total_start, total_end);
    
    return 0;
}
