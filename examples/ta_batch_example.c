/* Example demonstrating TA-Lib parallel batch execution API
 *
 * This example shows how to:
 * 1. Initialize batch configuration
 * 2. Execute parallel vector operations
 * 3. Query system capabilities
 */

#include <stdio.h>
#include <stdlib.h>
#include "ta_libc.h"
#include "ta_batch.h"

#define DATA_SIZE 100000

int main(void)
{
    TA_RetCode retCode;
    TA_BatchConfig config;
    double *arrayA, *arrayB, *result;
    int i;
    
    printf("TA-Lib Parallel Batch Execution Example\n");
    printf("========================================\n\n");
    
    /* Initialize TA-Lib */
    retCode = TA_Initialize();
    if( retCode != TA_SUCCESS )
    {
        printf("TA_Initialize failed: %d\n", retCode);
        return 1;
    }
    
    /* Query system capabilities */
    printf("System Information:\n");
    printf("  Hardware threads: %d\n", TA_GetHardwareThreadCount());
    printf("  NUMA nodes: %d\n", TA_GetNumaNodeCount());
    printf("  OpenMP available: %s\n\n", TA_IsOpenMPAvailable() ? "Yes" : "No");
    
    /* Initialize batch configuration */
    TA_BatchConfig_Init(&config);
    printf("Batch Configuration:\n");
    printf("  Default threads: %d\n", TA_BatchConfig_GetThreadCount(&config));
    printf("  Max threads: %d\n\n", TA_BATCH_MAX_THREADS);
    
    /* Allocate test data */
    arrayA = (double*)malloc(DATA_SIZE * sizeof(double));
    arrayB = (double*)malloc(DATA_SIZE * sizeof(double));
    result = (double*)malloc(DATA_SIZE * sizeof(double));
    
    if( !arrayA || !arrayB || !result )
    {
        printf("Memory allocation failed\n");
        free(arrayA);
        free(arrayB);
        free(result);
        TA_Shutdown();
        return 1;
    }
    
    /* Initialize test data */
    for( i = 0; i < DATA_SIZE; i++ )
    {
        arrayA[i] = (double)i;
        arrayB[i] = (double)(i * 2);
    }
    
    /* Test parallel vector operations */
    printf("Testing parallel vector operations on %d elements...\n\n", DATA_SIZE);
    
    /* Addition */
    TA_ParallelVecAdd(arrayA, arrayB, result, DATA_SIZE, &config);
    printf("Parallel Add: result[0] = %.2f (expected %.2f)\n", 
           result[0], arrayA[0] + arrayB[0]);
    printf("              result[%d] = %.2f (expected %.2f)\n\n",
           DATA_SIZE-1, result[DATA_SIZE-1], 
           arrayA[DATA_SIZE-1] + arrayB[DATA_SIZE-1]);
    
    /* Subtraction */
    TA_ParallelVecSub(arrayA, arrayB, result, DATA_SIZE, &config);
    printf("Parallel Sub: result[0] = %.2f (expected %.2f)\n",
           result[0], arrayA[0] - arrayB[0]);
    printf("              result[%d] = %.2f (expected %.2f)\n\n",
           DATA_SIZE-1, result[DATA_SIZE-1],
           arrayA[DATA_SIZE-1] - arrayB[DATA_SIZE-1]);
    
    /* Multiplication */
    TA_ParallelVecMul(arrayA, arrayB, result, DATA_SIZE, &config);
    printf("Parallel Mul: result[0] = %.2f (expected %.2f)\n",
           result[0], arrayA[0] * arrayB[0]);
    printf("              result[%d] = %.2f (expected %.2f)\n\n",
           DATA_SIZE-1, result[DATA_SIZE-1],
           arrayA[DATA_SIZE-1] * arrayB[DATA_SIZE-1]);
    
    /* Division */
    TA_ParallelVecDiv(arrayB, arrayA, result, DATA_SIZE, &config);
    printf("Parallel Div: result[1] = %.2f (expected %.2f)\n",
           result[1], arrayB[1] / arrayA[1]);
    printf("              result[%d] = %.2f (expected %.2f)\n\n",
           DATA_SIZE-1, result[DATA_SIZE-1],
           arrayB[DATA_SIZE-1] / arrayA[DATA_SIZE-1]);
    
    /* Test with custom thread count */
    config.numThreads = 4;
    printf("Testing with 4 threads (custom configuration)...\n");
    printf("  Effective threads: %d\n\n", TA_BatchConfig_GetThreadCount(&config));
    
    TA_ParallelVecAdd(arrayA, arrayB, result, DATA_SIZE, &config);
    printf("Parallel Add (4 threads): result[0] = %.2f\n\n", result[0]);
    
    /* Test batch execution API
     * Note: This is a placeholder - actual indicator dispatch not yet implemented
     */
    printf("Testing batch indicator execution...\n");
    TA_BatchJob jobs[2];
    
    jobs[0].functionName = "SMA";
    jobs[0].startIdx = 0;
    jobs[0].endIdx = DATA_SIZE - 1;
    jobs[0].optInInt0 = 20;  /* period */
    
    jobs[1].functionName = "RSI";
    jobs[1].startIdx = 0;
    jobs[1].endIdx = DATA_SIZE - 1;
    jobs[1].optInInt0 = 14;  /* period */
    
    retCode = TA_BatchExecute(&config, jobs, 2);
    printf("  Batch execute result: %d\n", retCode);
    printf("  Job[0] status: %d (TA_FUNC_NOT_FOUND expected - dispatch not yet implemented)\n", 
           jobs[0].retCode);
    printf("  Job[1] status: %d\n\n", jobs[1].retCode);
    
    /* Cleanup */
    free(arrayA);
    free(arrayB);
    free(result);
    
    TA_Shutdown();
    
    printf("Example completed successfully!\n");
    return 0;
}
