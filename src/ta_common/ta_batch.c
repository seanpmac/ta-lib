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

#include <string.h>
#include "ta_batch.h"
#include "ta_vec_math.h"

#ifdef _OPENMP
#include <omp.h>

static void TA_Internal_SelectSchedule(const TA_BatchConfig *config,
                                       int threads,
                                       int workItems,
                                       int defaultPolicy,
                                       omp_sched_t *outKind,
                                       int *outChunk)
{
    int policy = defaultPolicy;

    if( config )
    {
        switch( config->schedulePolicy )
        {
        case 1:
            policy = 1;
            break;
        case 2:
            policy = 2;
            break;
        default:
            policy = 0;
            break;
        }
    }

    omp_sched_t kind;
    switch( policy )
    {
    case 1:
        kind = omp_sched_dynamic;
        break;
    case 2:
        kind = omp_sched_guided;
        break;
    default:
        kind = omp_sched_static;
        break;
    }

    int chunk = (config && (config->chunkSize > 0)) ? config->chunkSize : 0;

    if( chunk <= 0 )
    {
        if( kind == omp_sched_static )
        {
            if( threads > 0 )
            {
                chunk = workItems / threads;
                if( chunk <= 0 )
                    chunk = 1;
            }
            else
            {
                chunk = 1;
            }
        }
        else
        {
            chunk = 1;
        }
    }

    *outKind = kind;
    *outChunk = chunk;
}
#endif

/* Initialize batch configuration with sensible defaults */
void TA_BatchConfig_Init( TA_BatchConfig *config )
{
    if( !config )
        return;
    
    config->numThreads = 0;  /* Auto-detect */
    config->numaNode = -1;   /* No affinity */
    config->schedulePolicy = 0;  /* Static scheduling */
    config->chunkSize = 0;   /* Auto chunk size */
}

/* Get hardware thread count */
int TA_GetHardwareThreadCount(void)
{
#ifdef _OPENMP
    return omp_get_max_threads();
#else
    return 1;  /* No OpenMP, assume single-threaded */
#endif
}

/* Get NUMA node count - simplified for now */
int TA_GetNumaNodeCount(void)
{
    /* TODO: Implement NUMA detection using hwloc or platform-specific APIs
     * For now, assume single NUMA domain (works for most systems)
     */
    return 1;
}

/* Check if OpenMP is available */
int TA_IsOpenMPAvailable(void)
{
#ifdef _OPENMP
    return 1;
#else
    return 0;
#endif
}

/* Get the actual thread count to use */
int TA_BatchConfig_GetThreadCount( const TA_BatchConfig *config )
{
    if( !config )
        return 1;
    
    int threads = config->numThreads;
    
    if( threads <= 0 )
    {
        /* Auto-detect: use hardware threads, capped at max */
        threads = TA_GetHardwareThreadCount();
        if( threads > TA_BATCH_MAX_THREADS )
            threads = TA_BATCH_MAX_THREADS;
    }
    else if( threads > TA_BATCH_MAX_THREADS )
    {
        /* Cap at maximum */
        threads = TA_BATCH_MAX_THREADS;
    }
    
    return threads;
}

/* Parallel vector addition with OpenMP */
void TA_ParallelVecAdd(
    const double *a,
    const double *b,
    double *dest,
    int count,
    const TA_BatchConfig *config
)
{
    if( count <= 0 || !a || !b || !dest )
        return;
    
#ifdef _OPENMP
    int threads = TA_BatchConfig_GetThreadCount(config);
    
    /* For small arrays, overhead of threading outweighs benefits */
    if( count < 10000 || threads == 1 )
    {
        TA_VEC_AddD(a, b, dest, count);
        return;
    }

    omp_sched_t prevKind;
    int prevChunk;
    omp_get_schedule(&prevKind, &prevChunk);

    omp_sched_t newKind;
    int newChunk;
    TA_Internal_SelectSchedule(config, threads, count, 0, &newKind, &newChunk);
    omp_set_schedule(newKind, newChunk);
    int chunk = newChunk;
    if( chunk <= 0 )
        chunk = (threads > 0) ? (count + threads - 1) / threads : count;
    if( chunk <= 0 )
        chunk = 1;

    /* Parallel execution with SIMD within each thread */
    #pragma omp parallel num_threads(threads)
    {
        const int chunkSize = chunk;
        #pragma omp for schedule(runtime)
        for( int start = 0; start < count; start += chunkSize )
        {
            int length = chunkSize;
            if( start + length > count )
                length = count - start;
            TA_VEC_AddD_Range(a, b, dest, start, length);
        }
    }

    omp_set_schedule(prevKind, prevChunk);
#else
    /* Fall back to optimized serial SIMD version */
    TA_VEC_AddD(a, b, dest, count);
#endif
}

/* Parallel vector subtraction */
void TA_ParallelVecSub(
    const double *a,
    const double *b,
    double *dest,
    int count,
    const TA_BatchConfig *config
)
{
    if( count <= 0 || !a || !b || !dest )
        return;
    
#ifdef _OPENMP
    int threads = TA_BatchConfig_GetThreadCount(config);
    
    if( count < 10000 || threads == 1 )
    {
        TA_VEC_SubD(a, b, dest, count);
        return;
    }

    omp_sched_t prevKind;
    int prevChunk;
    omp_get_schedule(&prevKind, &prevChunk);

    omp_sched_t newKind;
    int newChunk;
    TA_Internal_SelectSchedule(config, threads, count, 0, &newKind, &newChunk);
    omp_set_schedule(newKind, newChunk);
    int chunk = newChunk;
    if( chunk <= 0 )
        chunk = (threads > 0) ? (count + threads - 1) / threads : count;
    if( chunk <= 0 )
        chunk = 1;

    #pragma omp parallel num_threads(threads)
    {
        const int chunkSize = chunk;
        #pragma omp for schedule(runtime)
        for( int start = 0; start < count; start += chunkSize )
        {
            int length = chunkSize;
            if( start + length > count )
                length = count - start;
            TA_VEC_SubD_Range(a, b, dest, start, length);
        }
    }

    omp_set_schedule(prevKind, prevChunk);
#else
    TA_VEC_SubD(a, b, dest, count);
#endif
}

/* Parallel vector multiplication */
void TA_ParallelVecMul(
    const double *a,
    const double *b,
    double *dest,
    int count,
    const TA_BatchConfig *config
)
{
    if( count <= 0 || !a || !b || !dest )
        return;
    
#ifdef _OPENMP
    int threads = TA_BatchConfig_GetThreadCount(config);
    
    if( count < 10000 || threads == 1 )
    {
        TA_VEC_MulD(a, b, dest, count);
        return;
    }

    omp_sched_t prevKind;
    int prevChunk;
    omp_get_schedule(&prevKind, &prevChunk);

    omp_sched_t newKind;
    int newChunk;
    TA_Internal_SelectSchedule(config, threads, count, 0, &newKind, &newChunk);
    omp_set_schedule(newKind, newChunk);
    int chunk = newChunk;
    if( chunk <= 0 )
        chunk = (threads > 0) ? (count + threads - 1) / threads : count;
    if( chunk <= 0 )
        chunk = 1;

    #pragma omp parallel num_threads(threads)
    {
        const int chunkSize = chunk;
        #pragma omp for schedule(runtime)
        for( int start = 0; start < count; start += chunkSize )
        {
            int length = chunkSize;
            if( start + length > count )
                length = count - start;
            TA_VEC_MulD_Range(a, b, dest, start, length);
        }
    }

    omp_set_schedule(prevKind, prevChunk);
#else
    TA_VEC_MulD(a, b, dest, count);
#endif
}

/* Parallel vector division */
void TA_ParallelVecDiv(
    const double *a,
    const double *b,
    double *dest,
    int count,
    const TA_BatchConfig *config
)
{
    if( count <= 0 || !a || !b || !dest )
        return;
    
#ifdef _OPENMP
    int threads = TA_BatchConfig_GetThreadCount(config);
    
    if( count < 10000 || threads == 1 )
    {
        TA_VEC_DivD(a, b, dest, count);
        return;
    }

    omp_sched_t prevKind;
    int prevChunk;
    omp_get_schedule(&prevKind, &prevChunk);

    omp_sched_t newKind;
    int newChunk;
    TA_Internal_SelectSchedule(config, threads, count, 0, &newKind, &newChunk);
    omp_set_schedule(newKind, newChunk);
    int chunk = newChunk;
    if( chunk <= 0 )
        chunk = (threads > 0) ? (count + threads - 1) / threads : count;
    if( chunk <= 0 )
        chunk = 1;

    #pragma omp parallel num_threads(threads)
    {
        const int chunkSize = chunk;
        #pragma omp for schedule(runtime)
        for( int start = 0; start < count; start += chunkSize )
        {
            int length = chunkSize;
            if( start + length > count )
                length = count - start;
            TA_VEC_DivD_Range(a, b, dest, start, length);
        }
    }

    omp_set_schedule(prevKind, prevChunk);
#else
    TA_VEC_DivD(a, b, dest, count);
#endif
}

void TA_ParallelVecFma(
    const double *a,
    const double *b,
    const double *c,
    double *dest,
    int count,
    const TA_BatchConfig *config
)
{
    if( count <= 0 || !a || !b || !c || !dest )
        return;

#ifdef _OPENMP
    int threads = TA_BatchConfig_GetThreadCount(config);

    if( count < 10000 || threads == 1 )
    {
        TA_VEC_FmaD(a, b, c, dest, count);
        return;
    }

    omp_sched_t prevKind;
    int prevChunk;
    omp_get_schedule(&prevKind, &prevChunk);

    omp_sched_t newKind;
    int newChunk;
    TA_Internal_SelectSchedule(config, threads, count, 0, &newKind, &newChunk);
    omp_set_schedule(newKind, newChunk);
    int chunk = newChunk;
    if( chunk <= 0 )
        chunk = (threads > 0) ? (count + threads - 1) / threads : count;
    if( chunk <= 0 )
        chunk = 1;

    #pragma omp parallel num_threads(threads)
    {
        const int chunkSize = chunk;
        #pragma omp for schedule(runtime)
        for( int start = 0; start < count; start += chunkSize )
        {
            int length = chunkSize;
            if( start + length > count )
                length = count - start;
            TA_VEC_FmaD_Range(a, b, c, dest, start, length);
        }
    }

    omp_set_schedule(prevKind, prevChunk);
#else
    TA_VEC_FmaD(a, b, c, dest, count);
#endif
}

/* Batch execution of multiple indicators in parallel
 * 
 * This is a simplified initial implementation that executes jobs in parallel.
 * Future enhancements:
 * - Dynamic function dispatch based on functionName
 * - Shared computation caching (TRANGE, DI/DM)
 * - NUMA-aware memory allocation
 * - Work stealing for load balancing
 */
TA_RetCode TA_BatchExecute(
    const TA_BatchConfig *config,
    TA_BatchJob *jobs,
    int numJobs
)
{
    if( !config || !jobs || numJobs <= 0 )
        return TA_BAD_PARAM;
    
#ifdef _OPENMP
    int threads = TA_BatchConfig_GetThreadCount(config);
    
    /* For small batches or single thread, run serially */
    if( numJobs == 1 || threads == 1 )
    {
        /* Serial execution - job dispatch happens here
         * For now, just mark as not implemented since we need
         * function table lookup infrastructure
         */
        for( int i = 0; i < numJobs; ++i )
        {
            jobs[i].retCode = TA_FUNC_NOT_FOUND;
        }
        return TA_SUCCESS;
    }

    omp_sched_t prevKind;
    int prevChunk;
    omp_get_schedule(&prevKind, &prevChunk);

    omp_sched_t newKind;
    int newChunk;
    TA_Internal_SelectSchedule(config, threads, numJobs, 1, &newKind, &newChunk);
    omp_set_schedule(newKind, newChunk);
    
    /* Parallel execution with configurable scheduling */
    #pragma omp parallel num_threads(threads)
    {
        /* Each thread processes a subset of jobs */
        #pragma omp for schedule(runtime)
        for( int i = 0; i < numJobs; ++i )
        {
            /* TODO: Implement dynamic dispatch to actual indicator functions
             * This requires a function table mapping names to function pointers
             * For now, mark as not implemented
             */
            jobs[i].retCode = TA_FUNC_NOT_FOUND;
        }
    }

    omp_set_schedule(prevKind, prevChunk);

    return TA_SUCCESS;
#else
    /* No OpenMP - run serially */
    for( int i = 0; i < numJobs; ++i )
    {
        jobs[i].retCode = TA_FUNC_NOT_FOUND;
    }
    return TA_SUCCESS;
#endif
}
