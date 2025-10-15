/* TA-Lib Performance Profiling Harness - Main Entry Point */

#include "ta_perf.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void print_usage(const char *prog) {
    printf("TA-Lib Performance Profiling Harness\n\n");
    printf("Usage: %s [OPTIONS]\n\n", prog);
    printf("Options:\n");
    printf("  --input FILE        Path to CSV file with OHLCV data\n");
    printf("  --input-dir DIR     Directory containing multiple CSV files\n");
    printf("  --mode MODE         Execution mode: serial, parallel, both (default: both)\n");
    printf("  --threads N         Thread count for parallel mode (default: auto)\n");
    printf("  --max-threads N     Cap thread count (default: 128)\n");
    printf("  --output FILE       Output file (.json or .csv) (default: stdout)\n");
    printf("  --indicators LIST   Comma-separated indicator names (default: all)\n");
    printf("  --iterations N      Repeat count (default: 1)\n");
    printf("  --warmup N          Warm-up iterations (default: 1)\n");
    printf("  --verbose           Print detailed logs\n");
    printf("  --help              Show this help\n\n");
    printf("Examples:\n");
    printf("  %s --input data.csv --mode both --output results.json\n", prog);
    printf("  %s --input-dir data/ --threads 64 --output batch.csv\n", prog);
}

static void parse_args(int argc, char **argv, Config *config) {
    /* Set defaults */
    memset(config, 0, sizeof(Config));
    config->mode = MODE_BOTH;
    config->format = FORMAT_JSON;
    config->thread_count = 0;  /* 0 = auto-detect */
    config->max_threads = 128;
    config->iterations = 1;
    config->warmup = 1;
    config->verbose = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
            strncpy(config->input_file, argv[++i], MAX_PATH - 1);
        } else if (strcmp(argv[i], "--input-dir") == 0 && i + 1 < argc) {
            strncpy(config->input_dir, argv[++i], MAX_PATH - 1);
        } else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            const char *mode = argv[++i];
            if (strcmp(mode, "serial") == 0) config->mode = MODE_SERIAL;
            else if (strcmp(mode, "parallel") == 0) config->mode = MODE_PARALLEL;
            else if (strcmp(mode, "both") == 0) config->mode = MODE_BOTH;
            else {
                fprintf(stderr, "Unknown mode: %s\n", mode);
                exit(1);
            }
        } else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
            const char *threads = argv[++i];
            if (strcmp(threads, "auto") == 0) {
                config->thread_count = 0;
            } else {
                char *endptr = NULL;
                long val = strtol(threads, &endptr, 10);
                if (*endptr != '\0' || val <= 0) {
                    fprintf(stderr, "Invalid thread count: %s\n", threads);
                    exit(1);
                }
                config->thread_count = (int)val;
            }
        } else if (strcmp(argv[i], "--max-threads") == 0 && i + 1 < argc) {
            config->max_threads = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            strncpy(config->output_file, argv[++i], MAX_PATH - 1);
            if (strstr(config->output_file, ".csv")) {
                config->format = FORMAT_CSV;
            }
        } else if (strcmp(argv[i], "--indicators") == 0 && i + 1 < argc) {
            strncpy(config->indicator_filter, argv[++i], 511);
        } else if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            config->iterations = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
            config->warmup = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--verbose") == 0) {
            config->verbose = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            exit(1);
        }
    }

    /* Validate */
    if (config->input_file[0] == '\0' && config->input_dir[0] == '\0') {
        fprintf(stderr, "Error: --input or --input-dir required\n\n");
        print_usage(argv[0]);
        exit(1);
    }
}

int main(int argc, char **argv) {
    Config config;
    PriceData data;
    RunResult serial_result = {0};
    RunResult parallel_result = {0};
    FILE *output = stdout;

    parse_args(argc, argv, &config);
    timer_init();

    /* Load data */
    if (config.input_file[0] != '\0') {
        if (config.verbose) {
            printf("Loading data from %s...\n", config.input_file);
        }
        if (load_csv(config.input_file, &data) != 0) {
            fprintf(stderr, "Failed to load %s\n", config.input_file);
            return 1;
        }
        if (config.verbose) {
            printf("Loaded %d bars\n", data.bar_count);
        }
    } else {
        /* TODO: Multi-file batch loading */
        fprintf(stderr, "Error: --input-dir not yet implemented\n");
        return 1;
    }

    /* Warm-up runs */
    if (config.warmup > 0 && config.verbose) {
        printf("Running %d warm-up iteration(s)...\n", config.warmup);
    }
    for (int i = 0; i < config.warmup; i++) {
        RunResult dummy = {0};
        if (config.mode == MODE_SERIAL || config.mode == MODE_BOTH) {
            run_serial(&data, &config, &dummy);
        }
        if (config.mode == MODE_PARALLEL || config.mode == MODE_BOTH) {
            run_parallel(&data, &config, &dummy);
        }
    }

    /* Actual profiling runs */
    if (config.verbose) {
        printf("Running %d profiling iteration(s)...\n", config.iterations);
    }

    for (int iter = 0; iter < config.iterations; iter++) {
        if (config.mode == MODE_SERIAL || config.mode == MODE_BOTH) {
            RunResult iter_result = {0};
            if (run_serial(&data, &config, &iter_result) != 0) {
                fprintf(stderr, "Serial run failed\n");
                return 1;
            }
            /* Accumulate results */
            if (iter == 0) {
                serial_result = iter_result;
            } else {
                serial_result.total_time_us += iter_result.total_time_us;
                for (int i = 0; i < iter_result.indicator_count; i++) {
                    serial_result.indicators[i].total_time_us += iter_result.indicators[i].total_time_us;
                }
            }
        }

        if (config.mode == MODE_PARALLEL || config.mode == MODE_BOTH) {
            RunResult iter_result = {0};
            if (run_parallel(&data, &config, &iter_result) != 0) {
                fprintf(stderr, "Parallel run failed\n");
                return 1;
            }
            /* Accumulate results */
            if (iter == 0) {
                parallel_result = iter_result;
            } else {
                parallel_result.total_time_us += iter_result.total_time_us;
                for (int i = 0; i < iter_result.indicator_count; i++) {
                    parallel_result.indicators[i].total_time_us += iter_result.indicators[i].total_time_us;
                }
            }
        }
    }

    /* Average results if multiple iterations */
    if (config.iterations > 1) {
        serial_result.total_time_us /= config.iterations;
        parallel_result.total_time_us /= config.iterations;
        for (int i = 0; i < serial_result.indicator_count; i++) {
            serial_result.indicators[i].total_time_us /= config.iterations;
            serial_result.indicators[i].avg_time_us = 
                serial_result.indicators[i].total_time_us / serial_result.indicators[i].calls;
        }
        for (int i = 0; i < parallel_result.indicator_count; i++) {
            parallel_result.indicators[i].total_time_us /= config.iterations;
            parallel_result.indicators[i].avg_time_us = 
                parallel_result.indicators[i].total_time_us / parallel_result.indicators[i].calls;
        }
    }

    /* Open output file if specified */
    if (config.output_file[0] != '\0') {
        output = fopen(config.output_file, "w");
        if (!output) {
            fprintf(stderr, "Failed to open %s for writing\n", config.output_file);
            return 1;
        }
    }

    /* Generate report */
    if (config.format == FORMAT_JSON) {
        report_json(output, 
                    config.mode != MODE_PARALLEL ? &serial_result : NULL,
                    config.mode != MODE_SERIAL ? &parallel_result : NULL,
                    &data, &config);
    } else {
        report_csv(output,
                   config.mode != MODE_PARALLEL ? &serial_result : NULL,
                   config.mode != MODE_SERIAL ? &parallel_result : NULL);
    }

    if (output != stdout) {
        fclose(output);
        if (config.verbose) {
            printf("Results written to %s\n", config.output_file);
        }
    }

    free_price_data(&data);
    return 0;
}
