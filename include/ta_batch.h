/* TA-LIB Copyright (c) 1999-2025, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TA_BATCH_H
#define TA_BATCH_H

#ifndef TA_DEFS_H
   #include "ta_defs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Batching API for parallel indicator execution
 *
 * This API allows computing multiple indicators in parallel, leveraging
 * multi-core systems (6-10 cores on desktop, 64-128 cores on EPYC servers).
 *
 * Design principles:
 * - Thread-safe: Each batch job runs in isolated context
 * - NUMA-aware: Work can be partitioned across NUMA nodes (EPYC)
 * - Dynamic scheduling: Work stealing prevents idle cores
 * - Configurable: Max threads controllable (default: hardware_concurrency, cap: 128)
 */

/* Maximum number of threads supported (tuned for AMD EPYC 64-128 core systems) */
#define TA_BATCH_MAX_THREADS 128

/* Thread configuration for batch execution */
typedef struct {
    /* Number of worker threads to use.
     * 0 = auto-detect (min(hardware_concurrency, TA_BATCH_MAX_THREADS))
     * 1 = serial execution (no parallelization)
     * N = use N threads (capped at TA_BATCH_MAX_THREADS)
     */
    int numThreads;
    
    /* NUMA node affinity (optional, -1 for no affinity)
     * On multi-socket EPYC systems, bind threads to specific NUMA nodes
     * to minimize cross-socket memory traffic.
     */
    int numaNode;
    
    /* Scheduling policy:
     * 0 = static (default): divide work evenly upfront
     * 1 = dynamic: work stealing for load balancing
     * 2 = guided: start with large chunks, decrease over time
     */
    int schedulePolicy;
    
    /* Chunk size for dynamic/guided scheduling (0 = auto) */
    int chunkSize;
    
} TA_BatchConfig;

/* Initialize batch configuration with defaults */
TA_LIB_API void TA_BatchConfig_Init( TA_BatchConfig *config );

/* Get the number of threads that would be used with current config */
TA_LIB_API int TA_BatchConfig_GetThreadCount( const TA_BatchConfig *config );

/* Batch job descriptor for single indicator calculation */
typedef struct {
    /* Function name (e.g., "SMA", "RSI", "MACD") - used for dynamic dispatch */
    const char *functionName;
    
    /* Input parameters (indicator-specific) */
    int startIdx;
    int endIdx;
    
    /* Input arrays (up to 4 for OHLCV data) - shared across jobs */
    const double *inReal0;
    const double *inReal1;
    const double *inReal2;
    const double *inReal3;
    
    /* Optional integer parameters (periods, etc.) */
    int optInInt0;
    int optInInt1;
    int optInInt2;
    
    /* Optional double parameters (thresholds, etc.) */
    double optInDouble0;
    double optInDouble1;
    double optInDouble2;
    
    /* Output buffers (caller-allocated) */
    double *outReal0;
    double *outReal1;
    double *outReal2;
    
    int *outBegIdx;
    int *outNBElement;
    
    /* Result code (filled after execution) */
    TA_RetCode retCode;
    
} TA_BatchJob;

/* Execute multiple indicator calculations in parallel
 *
 * All jobs share the same input price arrays (inReal0-3 in each job),
 * which should be read-only. Each job has its own output buffers.
 *
 * This function is thread-safe and can be called concurrently from
 * different threads (each with their own batch of jobs).
 *
 * Returns:
 *   TA_SUCCESS if all jobs completed successfully
 *   TA_BAD_PARAM if config or jobs is NULL
 *   TA_ALLOC_ERR if internal memory allocation fails
 *   TA_INTERNAL_ERROR if thread pool initialization fails
 *
 * Note: Individual job errors are returned in job[i].retCode
 */
TA_LIB_API TA_RetCode TA_BatchExecute(
    const TA_BatchConfig *config,
    TA_BatchJob *jobs,
    int numJobs
);

/* Parallel array operations (embarrassingly parallel)
 * These operate on contiguous ranges of data with automatic chunking
 */

/* Parallel vector addition: dest[i] = a[i] + b[i] */
TA_LIB_API void TA_ParallelVecAdd(
    const double *a,
    const double *b,
    double *dest,
    int count,
    const TA_BatchConfig *config
);

/* Parallel vector subtraction: dest[i] = a[i] - b[i] */
TA_LIB_API void TA_ParallelVecSub(
    const double *a,
    const double *b,
    double *dest,
    int count,
    const TA_BatchConfig *config
);

/* Parallel vector multiplication: dest[i] = a[i] * b[i] */
TA_LIB_API void TA_ParallelVecMul(
    const double *a,
    const double *b,
    double *dest,
    int count,
    const TA_BatchConfig *config
);

/* Parallel vector division: dest[i] = a[i] / b[i] */
TA_LIB_API void TA_ParallelVecDiv(
    const double *a,
    const double *b,
    double *dest,
    int count,
    const TA_BatchConfig *config
);

/* Parallel fused multiply-add: dest[i] = a[i] * b[i] + c[i] */
TA_LIB_API void TA_ParallelVecFma(
    const double *a,
    const double *b,
    const double *c,
    double *dest,
    int count,
    const TA_BatchConfig *config
);

/* Query functions for runtime information */

/* Get hardware thread count (logical cores) */
TA_LIB_API int TA_GetHardwareThreadCount(void);

/* Get number of NUMA nodes (1 on non-NUMA systems) */
TA_LIB_API int TA_GetNumaNodeCount(void);

/* Check if OpenMP is available at runtime */
TA_LIB_API int TA_IsOpenMPAvailable(void);

#ifdef __cplusplus
}
#endif

#endif /* TA_BATCH_H */
