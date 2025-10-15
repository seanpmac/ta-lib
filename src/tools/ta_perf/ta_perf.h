/* TA-Lib Performance Profiling Harness
 * Copyright (c) 2025
 * Licensed under the same terms as TA-Lib
 */

#ifndef TA_PERF_H
#define TA_PERF_H

#include <stdint.h>
#include <stdio.h>

/* Configuration */
#define MAX_INDICATORS 192
#define MAX_PAIRS 1000
#define MAX_BARS 1000000
#define MAX_PATH 512

typedef enum {
    MODE_SERIAL,
    MODE_PARALLEL,
    MODE_BOTH
} ExecutionMode;

typedef enum {
    FORMAT_JSON,
    FORMAT_CSV
} OutputFormat;

/* CSV data structure */
typedef struct {
    char filename[MAX_PATH];
    int bar_count;
    double *open;
    double *high;
    double *low;
    double *close;
    double *volume;
} PriceData;

/* Per-indicator timing result */
typedef struct {
    char name[64];
    int calls;
    double total_time_us;
    double min_time_us;
    double max_time_us;
    double avg_time_us;
    int64_t bars_processed;
} IndicatorResult;

/* Aggregate results for a run */
typedef struct {
    ExecutionMode mode;
    int thread_count;
    double total_time_us;
    int indicator_count;
    IndicatorResult indicators[MAX_INDICATORS];
} RunResult;

/* Configuration */
typedef struct {
    char input_file[MAX_PATH];
    char input_dir[MAX_PATH];
    char output_file[MAX_PATH];
    char indicator_filter[512];
    ExecutionMode mode;
    OutputFormat format;
    int thread_count;
    int max_threads;
    int iterations;
    int warmup;
    int verbose;
} Config;

/* Function declarations */

/* csv_loader.c */
int load_csv(const char *path, PriceData *data);
void free_price_data(PriceData *data);

/* timer.c */
void timer_init(void);
uint64_t timer_now_us(void);
double timer_elapsed_us(uint64_t start, uint64_t end);

/* runner.c */
int run_serial(const PriceData *data, const Config *config, RunResult *result);
int run_parallel(const PriceData *data, const Config *config, RunResult *result);

/* reporter.c */
void report_json(FILE *out, const RunResult *serial, const RunResult *parallel, 
                 const PriceData *data, const Config *config);
void report_csv(FILE *out, const RunResult *serial, const RunResult *parallel);

#endif /* TA_PERF_H */
