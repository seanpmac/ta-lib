/* Reporter - Generate JSON and CSV output */

#include "ta_perf.h"
#include "ta_libc.h"
#include <time.h>
#include <string.h>

#define TIME_THRESHOLD_US 0.001

#define THROUGHPUT_TIME_THRESHOLD_US 0.001

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/utsname.h>
    #include <unistd.h>
#endif

/* Helper: get platform info */
static void get_platform_info(char *buf, size_t size) {
#ifdef _WIN32
    snprintf(buf, size, "Windows");
#else
    struct utsname uts;
    if (uname(&uts) == 0) {
        snprintf(buf, size, "%s %s", uts.sysname, uts.machine);
    } else {
        snprintf(buf, size, "Unknown");
    }
#endif
}

/* Helper: get CPU info (simplified) */
static void get_cpu_info(char *buf, size_t size) {
#ifdef _WIN32
    snprintf(buf, size, "Unknown CPU");
#else
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "model name", 10) == 0) {
                char *colon = strchr(line, ':');
                if (colon) {
                    char *name = colon + 2;
                    name[strcspn(name, "\n")] = 0;
                    snprintf(buf, size, "%s", name);
                    fclose(fp);
                    return;
                }
            }
        }
        fclose(fp);
    }
    snprintf(buf, size, "Unknown CPU");
#endif
}

/* Helper: get CPU core count */
static int get_cpu_cores(void) {
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

void report_json(FILE *out, const RunResult *serial, const RunResult *parallel,
                 const PriceData *data, const Config *config) {
    char platform[128];
    char cpu[256];
    int cores = get_cpu_cores();
    time_t now = time(NULL);
    char timestamp[64];
    
    get_platform_info(platform, sizeof(platform));
    get_cpu_info(cpu, sizeof(cpu));
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    
    fprintf(out, "{\n");
    fprintf(out, "  \"metadata\": {\n");
    fprintf(out, "    \"timestamp\": \"%s\",\n", timestamp);
    fprintf(out, "    \"platform\": \"%s\",\n", platform);
    fprintf(out, "    \"cpu_model\": \"%s\",\n", cpu);
    fprintf(out, "    \"cpu_cores\": %d,\n", cores);
    fprintf(out, "    \"ta_lib_version\": \"%s\",\n", TA_GetVersionString());
    fprintf(out, "    \"input_file\": \"%s\",\n", data->filename);
    fprintf(out, "    \"bar_count\": %d\n", data->bar_count);
    fprintf(out, "  }");
    
    if (serial) {
        fprintf(out, ",\n  \"serial\": {\n");
        fprintf(out, "    \"total_time_us\": %.1f,\n", serial->total_time_us);
        fprintf(out, "    \"indicators\": [\n");
        for (int i = 0; i < serial->indicator_count; i++) {
            const IndicatorResult *ind = &serial->indicators[i];
            fprintf(out, "      {\n");
            fprintf(out, "        \"name\": \"%s\",\n", ind->name);
            fprintf(out, "        \"calls\": %d,\n", ind->calls);
            fprintf(out, "        \"total_time_us\": %.1f,\n", ind->total_time_us);
            fprintf(out, "        \"avg_time_us\": %.1f,\n", ind->avg_time_us);
            
            if (ind->total_time_us > TIME_THRESHOLD_US) {  /* > 0.001μs */
                throughput = (ind->bars_processed * 1000000.0) / ind->total_time_us;
            }
                throughput = (ind->bars_processed * 1000000.0) / ind->total_time_us;
            }
            fprintf(out, "        \"throughput_bars_per_sec\": %.0f\n", throughput);
            
            fprintf(out, "      }%s\n", (i < serial->indicator_count - 1) ? "," : "");
        }
        fprintf(out, "    ]\n");
        fprintf(out, "  }");
    }
    
    if (parallel) {
        if (serial) fprintf(out, ",");
        fprintf(out, "\n  \"parallel\": {\n");
        fprintf(out, "    \"total_time_us\": %.1f,\n", parallel->total_time_us);
        fprintf(out, "    \"thread_count\": %d,\n", parallel->thread_count);
        if (serial) {
            double speedup = serial->total_time_us / parallel->total_time_us;
            double efficiency = speedup / parallel->thread_count;
            fprintf(out, "    \"speedup\": %.2f,\n", speedup);
            fprintf(out, "    \"efficiency\": %.3f,\n", efficiency);
        }
        fprintf(out, "    \"indicators\": [\n");
        for (int i = 0; i < parallel->indicator_count; i++) {
            const IndicatorResult *ind = &parallel->indicators[i];
            fprintf(out, "      {\n");
            fprintf(out, "        \"name\": \"%s\",\n", ind->name);
            fprintf(out, "        \"calls\": %d,\n", ind->calls);
            fprintf(out, "        \"total_time_us\": %.1f,\n", ind->total_time_us);
            fprintf(out, "        \"avg_time_us\": %.1f,\n", ind->avg_time_us);
            
            /* Calculate throughput, avoiding division by zero */
            double throughput = 0.0;
            if (ind->total_time_us > 0.001) {  /* > 0.001μs */
                throughput = (ind->bars_processed * 1000000.0) / ind->total_time_us;
            }
            fprintf(out, "        \"throughput_bars_per_sec\": %.0f\n", throughput);
            
            fprintf(out, "      }%s\n", (i < parallel->indicator_count - 1) ? "," : "");
        }
        fprintf(out, "    ]\n");
        fprintf(out, "  }");
    }
    
    fprintf(out, "\n}\n");
}

void report_csv(FILE *out, const RunResult *serial, const RunResult *parallel) {
    fprintf(out, "indicator,mode,calls,total_time_us,avg_time_us,throughput_bars_per_sec\n");
    
    if (serial) {
        for (int i = 0; i < serial->indicator_count; i++) {
            if (ind->total_time_us > TIME_THRESHOLD_US) {
                throughput = (ind->bars_processed * 1000000.0) / ind->total_time_us;
            }
                throughput = (ind->bars_processed * 1000000.0) / ind->total_time_us;
            }
            fprintf(out, "%s,serial,%d,%.1f,%.1f,%.0f\n",
                    ind->name, ind->calls, ind->total_time_us, ind->avg_time_us,
                    throughput);
        }
    }
    
    if (parallel) {
        for (int i = 0; i < parallel->indicator_count; i++) {
            const IndicatorResult *ind = &parallel->indicators[i];
            double throughput = 0.0;
            if (ind->total_time_us > 0.001) {
                throughput = (ind->bars_processed * 1000000.0) / ind->total_time_us;
            }
            fprintf(out, "%s,parallel,%d,%.1f,%.1f,%.0f\n",
                    ind->name, ind->calls, ind->total_time_us, ind->avg_time_us,
                    throughput);
        }
    }
}
