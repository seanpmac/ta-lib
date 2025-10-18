# TA-Lib Parallel Execution Guide

## Overview

TA-Lib now supports parallel execution of indicators using OpenMP, enabling significant performance improvements on multi-core systems ranging from desktop (6-10 cores) to high-end servers (64-128 cores on AMD EPYC).

## Features

### OpenMP Integration
- **SIMD Vectorization**: Automatic vectorization using `#pragma omp simd` for AVX2/AVX-512 (x86) and NEON (ARM)
- **Thread Parallelization**: Multi-threaded execution using `#pragma omp parallel for`
- **Graceful Degradation**: Falls back to optimized serial code when OpenMP is unavailable

### Batching API
- **TA_BatchExecute()**: Execute multiple indicators in parallel
- **TA_BatchConfig**: Configure thread count, scheduling policy, and NUMA affinity
- **Parallel Vector Operations**: Thread-parallel versions of ADD, SUB, MULT, DIV

### Dynamic Scheduling
- **Static Scheduling**: Divide work evenly upfront (default, best for uniform workloads)
- **Dynamic Scheduling**: Work stealing for load balancing (best for variable-length indicators)
- **Guided Scheduling**: Adaptive chunking that starts large and decreases over time

## Building with OpenMP Support

OpenMP support is automatically detected and enabled by CMake:

```bash
mkdir build
cd build
cmake ..
make
```

Check the build output for:
```
-- OpenMP enabled for ta-lib (SIMD and parallel execution support)
```

If OpenMP is not found, you'll see:
```
-- OpenMP not found - building without SIMD/parallel support
```

### Installing OpenMP

**macOS (Homebrew)**:
```bash
brew install libomp
```

**Ubuntu/Debian**:
```bash
sudo apt-get install libomp-dev
```

**Windows (MSVC)**:
OpenMP is included with Visual Studio. Ensure `/openmp` flag is set.

## Using the Batching API

### 1. System Capability Query

```c
#include "ta_batch.h"

/* Check system capabilities */
int threads = TA_GetHardwareThreadCount();
int numa_nodes = TA_GetNumaNodeCount();
int has_openmp = TA_IsOpenMPAvailable();

printf("Threads: %d, NUMA nodes: %d, OpenMP: %s\n",
       threads, numa_nodes, has_openmp ? "Yes" : "No");
```

### 2. Parallel Vector Operations

```c
#include "ta_batch.h"

/* Initialize configuration */
TA_BatchConfig config;
TA_BatchConfig_Init(&config);  /* Auto-detect threads */

/* Or customize */
config.numThreads = 8;         /* Use 8 threads */
config.schedulePolicy = 0;     /* Static scheduling */

/* Allocate arrays */
double *a = malloc(100000 * sizeof(double));
double *b = malloc(100000 * sizeof(double));
double *result = malloc(100000 * sizeof(double));

/* Parallel addition */
TA_ParallelVecAdd(a, b, result, 100000, &config);

/* Other operations */
TA_ParallelVecSub(a, b, result, 100000, &config);
TA_ParallelVecMul(a, b, result, 100000, &config);
TA_ParallelVecDiv(a, b, result, 100000, &config);
```

### 3. Batch Indicator Execution (Planned)

```c
/* NOTE: Full indicator dispatch not yet implemented
 * This shows the planned API
 */
TA_BatchJob jobs[3];

/* Job 0: SMA with period 20 */
jobs[0].functionName = "SMA";
jobs[0].startIdx = 0;
jobs[0].endIdx = dataSize - 1;
jobs[0].inReal0 = closePrice;
jobs[0].optInInt0 = 20;  /* period */
jobs[0].outReal0 = smaOutput;
jobs[0].outBegIdx = &outBegIdx0;
jobs[0].outNBElement = &outNBElement0;

/* Job 1: RSI with period 14 */
jobs[1].functionName = "RSI";
jobs[1].startIdx = 0;
jobs[1].endIdx = dataSize - 1;
jobs[1].inReal0 = closePrice;
jobs[1].optInInt0 = 14;  /* period */
jobs[1].outReal0 = rsiOutput;
jobs[1].outBegIdx = &outBegIdx1;
jobs[1].outNBElement = &outNBElement1;

/* Job 2: MACD */
jobs[2].functionName = "MACD";
jobs[2].startIdx = 0;
jobs[2].endIdx = dataSize - 1;
jobs[2].inReal0 = closePrice;
jobs[2].optInInt0 = 12;  /* fast period */
jobs[2].optInInt1 = 26;  /* slow period */
jobs[2].optInInt2 = 9;   /* signal period */
jobs[2].outReal0 = macdOutput;
jobs[2].outReal1 = macdSignal;
jobs[2].outReal2 = macdHist;

/* Execute all jobs in parallel */
TA_RetCode ret = TA_BatchExecute(&config, jobs, 3);

/* Check individual job results */
if (jobs[0].retCode == TA_SUCCESS) {
    /* SMA completed successfully */
}
```

## Performance Considerations

### When to Use Parallel Execution

**Good candidates:**
- Large datasets (>10,000 elements)
- Multiple independent indicators on shared data
- Embarrassingly parallel operations (math ops, price transforms)
- High-core-count systems (EPYC, Threadripper)

**Poor candidates:**
- Small datasets (<10,000 elements) - overhead exceeds benefits
- Single indicator calculation - no parallelism available
- Stateful indicators with dependencies - limited concurrency

### Automatic Thresholds

The parallel vector operations automatically choose between parallel and serial execution:

```c
/* Automatically uses parallel when count >= 10000 */
TA_ParallelVecAdd(a, b, result, count, &config);

/* Small arrays use optimized serial SIMD path */
/* Large arrays use multi-threaded parallel path */
```

### Thread Count Selection

```c
TA_BatchConfig config;
TA_BatchConfig_Init(&config);

/* Auto-detect (recommended) */
config.numThreads = 0;  
/* Uses min(hardware_threads, TA_BATCH_MAX_THREADS=128) */

/* Manual override */
config.numThreads = 16;  /* Use exactly 16 threads */

/* Serial execution (debugging) */
config.numThreads = 1;
```

### Scheduling Policies

```c
/* Static: divide work evenly upfront (default, best for uniform work) */
config.schedulePolicy = 0;

/* Dynamic: work stealing (best for variable-length indicators like MACD vs TRANGE) */
config.schedulePolicy = 1;

/* Guided: adaptive chunking (balance between static and dynamic) */
config.schedulePolicy = 2;
```

## NUMA Support (Planned)

On multi-socket systems (e.g., dual EPYC with 2 NUMA nodes):

```c
TA_BatchConfig config;
TA_BatchConfig_Init(&config);

/* Bind threads to NUMA node 0 */
config.numaNode = 0;

/* Let OS handle NUMA placement (default) */
config.numaNode = -1;
```

**Note**: NUMA node detection and binding not yet implemented. Currently returns 1 for all systems.

## Examples

See `examples/ta_batch_example.c` for a complete working example demonstrating:
- System capability queries
- Parallel vector operations
- Batch execution API (placeholder)

To build and run:
```bash
cd build
make
./examples/ta_batch_example
```

## Roadmap

### Completed ✓
- OpenMP integration in main library
- Batching API infrastructure
- Parallel vector operations (ADD/SUB/MULT/DIV)
- Dynamic scheduling support
- System query functions

### In Progress
- [ ] Indicator dispatch table for TA_BatchExecute()
- [ ] NUMA node detection and thread affinity
- [ ] Performance benchmarks on i9, M1 Max, and EPYC

### Planned
- [ ] Shared computation caching (TRANGE, DI/DM)
- [ ] Thread-safe state struct audit
- [ ] Buffer pool for lock-free memory management
- [ ] GPU offload for selected indicators (CUDA/Metal)

## Performance Targets

**Desktop (i9 6-core, M1 Max 10-core)**:
- 2-4× speedup on math operations with >100k elements
- Efficient scaling up to hardware thread count

**Server (EPYC 64-128 core)**:
- Linear scaling for embarrassingly parallel workloads
- 20-30× speedup on batch indicator execution
- NUMA-aware memory allocation to minimize cross-socket traffic

## Troubleshooting

**OpenMP not found during build:**
- Install OpenMP runtime library (see "Installing OpenMP" above)
- Set environment variables if needed:
  ```bash
  export LDFLAGS="-L/usr/local/opt/libomp/lib"
  export CPPFLAGS="-I/usr/local/opt/libomp/include"
  ```

**Performance worse than serial:**
- Check dataset size - small arrays have too much overhead
- Verify thread count isn't exceeding hardware threads
- Try different scheduling policies
- Profile with `ta_perf` to identify bottlenecks

**Build errors about omp.h:**
- OpenMP headers not in include path
- Add include directory to CPPFLAGS or CMake

## Further Reading

- [OpenMP Specification](https://www.openmp.org/specifications/)
- [NUMA Architecture](https://en.wikipedia.org/wiki/Non-uniform_memory_access)
- [SIMD Programming Guide](https://software.intel.com/content/www/us/en/develop/articles/practical-intel-avx-optimization-on-2nd-generation-intel-core-processors.html)

## Contributing

We welcome contributions to improve parallel execution:
- Implement indicator dispatch table
- Add NUMA detection (hwloc integration)
- Optimize scheduling policies
- Benchmark on new hardware platforms

See `AGENT_PLAN.md` for detailed technical roadmap.
