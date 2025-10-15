/* CSV Loader - Parses OHLCV data files */

#include "ta_perf.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define LINE_BUF_SIZE 4096

/* Helper: trim whitespace */
static char* trim(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    *(end+1) = 0;
    return str;
}

/* Detect file format by inspecting first non-header line */
static int detect_format(const char *line, int *format_type) {
    char line_copy[LINE_BUF_SIZE];
    strncpy(line_copy, line, LINE_BUF_SIZE - 1);
    
    /* Count commas to determine column count */
    int comma_count = 0;
    for (const char *p = line_copy; *p; p++) {
        if (*p == ',') comma_count++;
    }
    
    /* Check if first field looks like timestamp string vs number */
    char *token = strtok(line_copy, ",");
    if (!token) return -1;
    
    int has_date_chars = (strchr(token, '-') || strchr(token, '/') || strchr(token, ':')) ? 1 : 0;
    
    if (comma_count == 10 && !has_date_chars) {
        /* QuantConnect format: ms,bid_ohlc,bid_vol,ask_ohlc,ask_vol (11 columns) */
        *format_type = 1;
    } else {
        /* Standard format: [timestamp,]ohlc[,volume] */
        *format_type = 0;
    }
    
    return 0;
}

/* Parse CSV line into OHLCV values - supports multiple formats */
static int parse_line(char *line, double *open, double *high, double *low, 
                      double *close, double *volume, int format_type) {
    char *token;
    
    if (format_type == 1) {
        /* QuantConnect format: ms,bid_o,bid_h,bid_l,bid_c,bid_vol,ask_o,ask_h,ask_l,ask_c,ask_vol */
        /* Skip milliseconds */
        token = strtok(line, ",");
        if (!token) return -1;
        
        /* Use bid prices (columns 2-5) */
        token = strtok(NULL, ",");
        if (!token) return -1;
        *open = strtod(trim(token), NULL);
        
        token = strtok(NULL, ",");
        if (!token) return -1;
        *high = strtod(trim(token), NULL);
        
        token = strtok(NULL, ",");
        if (!token) return -1;
        *low = strtod(trim(token), NULL);
        
        token = strtok(NULL, ",");
        if (!token) return -1;
        *close = strtod(trim(token), NULL);
        
        /* Skip bid volume, ask prices/volume */
        *volume = 0.0;
        
    } else {
        /* Standard format with optional timestamp prefix */
        token = strtok(line, ",");
        if (!token) return -1;
        
        /* Try to parse as number - if it fails, assume it's timestamp */
        char *endptr;
        double val = strtod(trim(token), &endptr);
        if (*endptr != '\0') {
            /* Not a number, skip to next field */
            token = strtok(NULL, ",");
            if (!token) return -1;
        }
        
        /* Parse OHLC */
        *open = strtod(trim(token), NULL);
        
        token = strtok(NULL, ",");
        if (!token) return -1;
        *high = strtod(trim(token), NULL);
        
        token = strtok(NULL, ",");
        if (!token) return -1;
        *low = strtod(trim(token), NULL);
        
        token = strtok(NULL, ",");
        if (!token) return -1;
        *close = strtod(trim(token), NULL);
        
        /* Optional volume */
        token = strtok(NULL, ",");
        if (token) {
            *volume = strtod(trim(token), NULL);
        } else {
            *volume = 0.0;
        }
    }
    
    return 0;
}

int load_csv(const char *path, PriceData *data) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Cannot open %s\n", path);
        return -1;
    }
    
    memset(data, 0, sizeof(PriceData));
    strncpy(data->filename, path, MAX_PATH - 1);
    
    char line[LINE_BUF_SIZE];
    int has_volume = 0;
    int has_header = 0;
    int format_type = 0;  /* 0=standard, 1=QuantConnect */
    int capacity = 10000;  /* Initial allocation */
    int count = 0;
    
    /* Allocate initial buffers */
    data->open = malloc(capacity * sizeof(double));
    data->high = malloc(capacity * sizeof(double));
    data->low = malloc(capacity * sizeof(double));
    data->close = malloc(capacity * sizeof(double));
    data->volume = malloc(capacity * sizeof(double));
    
    if (!data->open || !data->high || !data->low || !data->close || !data->volume) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(fp);
        return -1;
    }
    
    /* Read lines */
    int first_data_line = 1;
    while (fgets(line, sizeof(line), fp)) {
        /* Trim newline */
        line[strcspn(line, "\r\n")] = 0;
        
        /* Skip empty lines */
        if (strlen(trim(line)) == 0) continue;
        
        /* Detect header row */
        if (!has_header && (strstr(line, "open") || strstr(line, "Open") || 
                            strstr(line, "timestamp") || strstr(line, "Timestamp"))) {
            has_header = 1;
            has_volume = (strstr(line, "volume") || strstr(line, "Volume")) ? 1 : 0;
            continue;
        }
        
        /* Detect format from first data line */
        if (first_data_line) {
            if (detect_format(line, &format_type) != 0) {
                fprintf(stderr, "Failed to detect format\n");
                fclose(fp);
                return -1;
            }
            first_data_line = 0;
        }
        
        /* Grow arrays if needed */
        if (count >= capacity) {
            int new_capacity = capacity * 2;
            if (new_capacity > MAX_BARS) {
                fprintf(stderr, "Data exceeds MAX_BARS limit (%d)\n", MAX_BARS);
                fclose(fp);
                return -1;
            }
            capacity = new_capacity;
            data->open = realloc(data->open, capacity * sizeof(double));
            data->high = realloc(data->high, capacity * sizeof(double));
            data->low = realloc(data->low, capacity * sizeof(double));
            data->close = realloc(data->close, capacity * sizeof(double));
            data->volume = realloc(data->volume, capacity * sizeof(double));
            
            if (!data->open || !data->high || !data->low || !data->close || !data->volume) {
                fprintf(stderr, "Memory reallocation failed\n");
                fclose(fp);
                return -1;
            }
        }
        
        /* Parse line */
        char line_copy[LINE_BUF_SIZE];
        strncpy(line_copy, line, LINE_BUF_SIZE - 1);
        if (parse_line(line_copy, &data->open[count], &data->high[count],
                       &data->low[count], &data->close[count],
                       &data->volume[count], format_type) == 0) {
            count++;
        }
    }
    
    fclose(fp);
    data->bar_count = count;
    
    if (count == 0) {
        fprintf(stderr, "No valid data found in %s\n", path);
        return -1;
    }
    
    return 0;
}

void free_price_data(PriceData *data) {
    if (data->open) free(data->open);
    if (data->high) free(data->high);
    if (data->low) free(data->low);
    if (data->close) free(data->close);
    if (data->volume) free(data->volume);
    memset(data, 0, sizeof(PriceData));
}
