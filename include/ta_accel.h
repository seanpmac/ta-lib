/*
 * Internal acceleration helpers for TA-Lib.
 *
 * These functions provide optional GPU/NPU acceleration for
 * selected indicators when the platform supports it. They
 * are not part of the public API but are exposed so that
 * multiple translation units inside the core library can
 * cooperate.
 */
#ifndef TA_ACCEL_H
#define TA_ACCEL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void TA_accel_init(void);
void TA_accel_shutdown(void);
bool TA_accel_is_ready(void);

bool TA_accel_sma_double(const double *inReal,
                         int optInTimePeriod,
                         int startIdx,
                         int endIdx,
                         int lookbackTotal,
                         double *outReal);

bool TA_accel_sma_float(const float *inReal,
                        int optInTimePeriod,
                        int startIdx,
                        int endIdx,
                        int lookbackTotal,
                        double *outReal);

bool TA_accel_trima_double(const double *inReal,
                           int optInTimePeriod,
                           int startIdx,
                           int endIdx,
                           int lookbackTotal,
                           double *outReal);

bool TA_accel_wma_double(const double *inReal,
                         int optInTimePeriod,
                         int startIdx,
                         int endIdx,
                         int lookbackTotal,
                         double *outReal);

#ifdef __cplusplus
}
#endif

#endif
