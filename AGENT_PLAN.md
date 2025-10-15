# TA-Lib Indicator Modernization Plan

_Last updated: 2025-10-14_

## Mission Snapshot
- **Scope:** 102 production indicators covering overlap studies, momentum/volume analytics, candlestick pattern recognition, statistical transforms, and math helpers.
- **Generated Surface:** Core C implementations under `src/ta_func/`, XML metadata in `ta_func_api.xml`, and auto-generated bindings (C#, Java, Rust, SWIG).
- **Objective:** Maximize performance across current hardware (i9/RTX 2080 Windows/Linux, M1 Max macOS) and future high-core-count AMD EPYC (64-128 cores) via SIMD, aggressive parallelization, shared caching, and selective GPU offload—keeping the C11 core lightweight and portable.
- **Execution Mandate:** Modernize first (optimize algorithms, infrastructure, profiling), then port additional indicators from QuantConnect.Lean and DaveSkenders.StockIndicators against the tuned baseline.

## Global Initiatives (Cross-Cutting)

### Performance & Profiling Infrastructure
- [ ] **Baseline Profiling Suite** – Establish reference datasets (50k, 100k, 500k bars) and capture current timings for all 102 indicators on i9/RTX 2080, M1 Max, and (when available) EPYC 64-128 core systems. Store baselines in version control; fail CI if regressions exceed 5%.
  - **Tool:** `ta_perf` - New profiling harness that accepts CSV data, measures serial vs parallel execution, outputs JSON/CSV metrics.
  - **Location:** `src/tools/ta_perf/` with standalone CMake build and integration into main build via `BUILD_DEV_TOOLS`.
  - **Usage:** `./ta_perf --input data.csv --mode both --threads 128 --output baseline.json`
- [ ] **Per-Indicator Microbenchmarks** – Extend `ta_perf` to support filtering specific indicators, multiple iterations, and warm-up runs for statistical stability.
- [ ] **Profiler Integration** – Wire platform-specific profiling (Intel VTune on i9, AMD µProf on EPYC, Instruments on M1) into dev workflows for hotspot identification and NUMA/cache analysis on high-core systems.

### Parallelization & Threading
- [ ] **Batching API** – Add public entry points that accept array-of-indicators + shared input buffers, distributing work across threads with configurable max workers (default: `min(hardware_concurrency, 128)` to scale from i9 to EPYC).
- [ ] **Thread-Safe State Structs** – Audit and annotate stateful (`_State`) implementations for thread safety; document which indicators can run concurrently vs require serialization.
- [ ] **OpenMP Pragmas** – Annotate embarrassingly parallel loops (math ops, bands, price transforms) with `#pragma omp simd` and `#pragma omp parallel for`, tuning chunk sizes for both low-core (6-10) and high-core (64-128) systems.
- [ ] **NUMA Awareness (EPYC)** – On multi-socket EPYC, bind worker threads to NUMA nodes and partition input data accordingly to minimize cross-socket memory traffic; expose tuning knobs in the .NET wrapper.
- [ ] **Work Stealing & Load Balancing** – Implement dynamic task queues so shorter-running indicators don't leave cores idle while longer ones finish; benchmark against static round-robin on EPYC hardware.

### SIMD & Vectorization
- [ ] **Runtime Dispatch** – Implement CPU feature detection (SSE2/SSSE3/AVX2/AVX-512 on x86; NEON on ARM) and function pointer dispatch to optimal kernels.
- [ ] **Priority Targets** – Vectorize SMA/EMA/WMA, price transforms (AVGPRICE, TYPPRICE), math operators (ADD/SUB/MULT/DIV), and TRANGE first; measure speedup per platform.
- [ ] **Intrinsic Wrappers** – Create portable SIMD abstraction layer (`ta_simd.h`) with fallback scalar paths to maintain compatibility.

### GPU Acceleration (Selective)
- [ ] **GPU Feasibility Matrix** – Document which indicators justify GPU offload (large batch, minimal branching, high arithmetic intensity) vs remain CPU-only.
- [ ] **ILGPU Pilot (.NET 9 wrapper)** – Prototype CUDA kernels for bands (BBANDS, ACCBANDS), math transforms, and arithmetic ops; expose via optional `UseGpuIfAvailable` flag with automatic fallback.
- [ ] **Metal Backend (M1 Max)** – Experiment with Metal compute shaders for the same indicator subset; measure data-transfer overhead vs compute gain.
- [ ] **Capability Detection** – Add runtime checks in wrappers to detect GPU availability and silently fall back to optimized CPU path when absent.

### Shared Computation & Caching
- [ ] **Dependency Graph** – Model shared primitives (TRANGE, DI/DM, rolling sums/means) and build a caching layer so multiple indicators reuse precomputed arrays.
- [ ] **Buffer Pool** – Implement thread-local or lock-free buffer pools to eliminate per-call allocations in hot paths.
- [ ] **Stateful Streaming API Audit** – Exercise `_State` entry points for every indicator; add regression fixtures ensuring incremental updates match batch results and leverage cached state efficiently.

### Metadata, Validation & Tooling
- [ ] **Metadata Hardening** – Validate optional parameter defaults and ranges in `ta_func_api.xml`; ensure docs/changelogs stay synchronized.
- [ ] **External Indicator Porting** – Stand up a reproducible harness to port and validate indicators sourced from QuantConnect.Lean and DaveSkenders.StockIndicators while keeping TA-Lib the performance baseline.
- [ ] **Documentation & Samples** – Expand recipe coverage with per-category how-to guides, including unit test examples, wrapper usage notes, and performance tuning tips.

## Category Deep Dives
Each section lists the indicators inspected, observations, and suggested work packages.

### Overlap Studies & Smoothing
**Indicators:** `ACCBANDS`, `ALLIGATOR`, `BBANDS`, `DEMA`, `EMA`, `KAMA`, `MA`, `MACD`, `MACDEXT`, `MACDFIX`, `MAMA`, `MAVP`, `MIDPOINT`, `SMA`, `T3`, `TEMA`, `TRIMA`, `WMA`

- _Observations:_ Heavy reuse of moving-average kernels; `ALLIGATOR` introduces multi-offset smoothing that can be generalized.
- _Next actions:_
  - [ ] Consolidate smoothing kernels into reusable helpers with vectorized backends.
  - [ ] Add regression data covering edge offsets (e.g., negative shifts) for `ALLIGATOR`, `MACDEXT`, `MAVP`.
  - [ ] Validate MA type switches across wrappers; ensure Rust generator emits ergonomic enums.

### Price & Typical Price Transforms
**Indicators:** `AVGPRICE`, `MEDPRICE`, `MIDPRICE`, `TYPPRICE`, `WCLPRICE`

- _Observations:_ Simple algebraic combinations; currently scalar loops.
- _Next actions:_
  - [ ] Auto-vectorize via `#pragma omp simd`, generating fused loads.
  - [ ] Provide inline functions the other indicators can call to avoid manual recomputation.

### Momentum & Trend Analytics
**Indicators:** `ADX`, `ADXR`, `APO`, `AROON`, `AROONOSC`, `BOP`, `CCI`, `CMO`, `DX`, `IMI`, `MINUS_DI`, `MINUS_DM`, `MOM`, `PLUS_DI`, `PLUS_DM`, `PPO`, `ROC`, `ROCP`, `ROCR`, `ROCR100`, `RSI`, `SAR`, `SAREXT`, `STOCH`, `STOCHF`, `STOCHRSI`, `TRIX`, `ULTOSC`, `WILLR`

- _Observations:_ Many depend on rolling highs/lows or DI calculations; `SAR/SAREXT` maintain elaborate state.
- _Next actions:_
  - [ ] Share precomputed DI/DM arrays across `ADX`, `ADXR`, `MINUS/PLUS` families.
  - [ ] Benchmark and potentially rewrite `SAR/SAREXT` using preallocated structs for cache locality.
  - [ ] Cover edge-case periods (e.g., `optInTimePeriod=2`) in tests.

### Volume & Flow Indicators
**Indicators:** `AD`, `ADOSC`, `MFI`, `OBV`

- _Observations:_ Depend on volume availability; some fallback silently when volume is zero.
- _Next actions:_
  - [ ] Document behaviour for missing volume and add validation hooks.
  - [ ] Explore prefix-sum acceleration for cumulative flows.

### Volatility & Range Metrics
**Indicators:** `ATR`, `NATR`, `TRANGE`, `STDDEV`, `VAR`, `AVGDEV`, `BETA`

- _Observations:_ `TRANGE` underpins ATR/NATR; `STDDEV/VAR/AVGDEV/BETA` share rolling statistics.
- _Next actions:_
  - [ ] Implement Welford-style numerically stable accumulators.
  - [ ] Cache `TRANGE` results for consumers; publish through shared buffer APIs.

### Statistical & Regression Tools
**Indicators:** `CORREL`, `LINEARREG`, `LINEARREG_ANGLE`, `LINEARREG_INTERCEPT`, `LINEARREG_SLOPE`, `TSF`

- _Observations:_ Linear algebra heavy; ripe for BLAS acceleration.
- _Next actions:_
  - [ ] Optional dependency on Accelerate/MKL for large-window regressions.
  - [ ] Provide double-check tests comparing CPU vs BLAS outputs.

### Cycle & Hilbert Suite
**Indicators:** `HT_DCPERIOD`, `HT_DCPHASE`, `HT_PHASOR`, `HT_SINE`, `HT_TRENDLINE`, `HT_TRENDMODE`

- _Observations:_ Intricate recursive coefficients; currently scalar.
- _Next actions:_
  - [ ] Profile with high-resolution timers; identify hot loops for vectorization.
  - [ ] Document mathematical derivations to aid future rewrites.

### Volume-Adjusted Extremes & Ranging
**Indicators:** `MAX`, `MAXINDEX`, `MIN`, `MININDEX`, `MINMAX`, `MINMAXINDEX`

- _Observations:_ Sliding window min/max operations with linear scans.
- _Next actions:_
  - [ ] Replace with deque-based O(1) window extrema for large periods.
  - [ ] Ensure index outputs stay consistent under ties.

### Math Operators & Arithmetic Vector Ops
**Indicators:** `ADD`, `DIV`, `MULT`, `SUB`, `SUM`

- _Observations:_ Straightforward array ops; ideal for SIMD/GPU.
- _Next actions:_
  - [ ] Implement shared vector math backend (SIMD + fallback).
  - [ ] Provide fused operations (e.g., multiply-add) to cut passes.

### Math Transforms (Scalar -> Vector)
**Indicators:** `ACOS`, `ASIN`, `ATAN`, `CEIL`, `COS`, `COSH`, `EXP`, `FLOOR`, `LN`, `LOG10`, `SIN`, `SINH`, `SQRT`, `TAN`, `TANH`

- _Observations:_ One-line wrappers around `math.h`; overhead dominated by call dispatch.
- _Next actions:_
  - [ ] Evaluate batching multiple transforms together to amortize overhead.
  - [ ] Consider vectorized `svml`/`sleef` integrations for performance builds.

### Price & Volume Oscillators (Percent-based)
**Indicators:** `APO`, `PPO`, `ROC`, `ROCP`, `ROCR`, `ROCR100`

- _Observations:_ Shared ratio computations.
- _Next actions:_
  - [ ] Guard against division by zero with consistent epsilon policy.
  - [ ] Expose configuration hooks for decimal precision across bindings.

### Candlestick Pattern Recognition
**Indicators:** `CDL2CROWS`, `CDL3BLACKCROWS`, `CDL3INSIDE`, `CDL3LINESTRIKE`, `CDL3OUTSIDE`, `CDL3STARSINSOUTH`, `CDL3WHITESOLDIERS`, `CDLABANDONEDBABY`, `CDLADVANCEBLOCK`, `CDLBELTHOLD`, `CDLBREAKAWAY`, `CDLCLOSINGMARUBOZU`, `CDLCONCEALBABYSWALL`, `CDLCOUNTERATTACK`, `CDLDARKCLOUDCOVER`, `CDLDOJI`, `CDLDOJISTAR`, `CDLDRAGONFLYDOJI`, `CDLENGULFING`, `CDLEVENINGDOJISTAR`, `CDLEVENINGSTAR`, `CDLGAPSIDESIDEWHITE`, `CDLGRAVESTONEDOJI`, `CDLHAMMER`, `CDLHANGINGMAN`, `CDLHARAMI`, `CDLHARAMICROSS`, `CDLHIGHWAVE`, `CDLHIKKAKE`, `CDLHIKKAKEMOD`, `CDLHOMINGPIGEON`, `CDLIDENTICAL3CROWS`, `CDLINNECK`, `CDLINVERTEDHAMMER`, `CDLKICKING`, `CDLKICKINGBYLENGTH`, `CDLLADDERBOTTOM`, `CDLLONGLEGGEDDOJI`, `CDLLONGLINE`, `CDLMARUBOZU`, `CDLMATCHINGLOW`, `CDLMATHOLD`, `CDLMORNINGDOJISTAR`, `CDLMORNINGSTAR`, `CDLONNECK`, `CDLPIERCING`, `CDLRICKSHAWMAN`, `CDLRISEFALL3METHODS`, `CDLSEPARATINGLINES`, `CDLSHOOTINGSTAR`, `CDLSHORTLINE`, `CDLSPINNINGTOP`, `CDLSTALLEDPATTERN`, `CDLSTICKSANDWICH`, `CDLTAKURI`, `CDLTASUKIGAP`, `CDLTHRUSTING`, `CDLTRISTAR`, `CDLUNIQUE3RIVER`, `CDLUPSIDEGAP2CROWS`, `CDLXSIDEGAP3METHODS`

- _Observations:_ Extensive use of lookback windows and `_CandleSettingType`; CPU hotspots due to branching.
- _Next actions:_
  - [ ] Refactor shared candle logic into templated helpers to reduce divergence.
  - [ ] Expand unit tests to cover exotic cases (e.g., gap handling, equal opens).
  - [ ] Generate visual examples automatically for documentation.

### Vector Oscillators & Balance Metrics
**Indicators:** `BOP`, `MFI`, `OBV`

- _Observations:_ Mix price and volume arrays; some rely on signed volumes.
- _Next actions:_
  - [ ] Sanity-check negative/zero volume handling.
  - [ ] Provide normalized outputs for machine learning pipelines.

### Remaining Utilities
**Indicators:** `BETA`, `IMI`, `NATR`, `OBV`, `SAR`, `SAREXT`, `SUM`

- _Observations:_ Already covered where applicable; ensure wrappers maintain consistent naming.
- _Next actions:_
  - [ ] Audit wrapper code generation for optional outputs (`SUM`, `SAR`).

### External Indicator Porting & Alignment

- _Targets:_ Gaps between TA-Lib and QuantConnect.Lean, DaveSkenders.StockIndicators, plus any bespoke client-side studies.
- _Next actions:_
  - [ ] Build a conversion checklist (naming, parameter conventions, lookback/output shape) before porting each indicator.
  - [ ] Use the profiling harness to capture TA-Lib vs source-library performance on shared datasets, documenting deltas.
  - [ ] Create regression fixtures covering both batch and stateful modes to guarantee parity with the original libraries.
  - [ ] Feed new indicators through metadata generators (`ta_func_api.xml`, bindings) and extend wrapper samples (including the .NET 9 branch).

## Execution Checklist for Future Agents
1. **Baseline & profile** – Capture current performance on i9/RTX 2080, M1 Max, and (when available) EPYC 64-128 core systems using `ta_regtest -p` with 50k/100k/500k datasets; commit baselines and NUMA topologies.
2. **SIMD first pass** – Vectorize SMA/EMA/WMA, price transforms, math operators, and TRANGE with runtime dispatch; validate speedups ≥2× on representative workloads across all target platforms.
3. **Shared caching layer** – Implement dependency graph for TRANGE/DI/DM; measure allocation reduction and multi-indicator batch speedups.
4. **Parallelization (scaling to EPYC)** – Add batching APIs with OpenMP threading (default max `hardware_concurrency`, capped at 128); implement NUMA-aware thread binding, dynamic work stealing, and tune chunk sizes for both low-core (6-10) and high-core (64-128) systems. Profile context-switch overhead and memory bandwidth saturation.
5. **Stateful audit** – Exercise `_State` paths for each indicator category; ensure incremental updates match batch results and exploit cached buffers efficiently.
6. **GPU pilot** – Prototype ILGPU (CUDA) and Metal backends for bands/math ops; document transfer overhead vs compute gain and set capability detection defaults. Re-evaluate GPU priority once EPYC parallelization is tuned (CPU may outpace GPU for many workloads).
7. **Port external indicators** – After core optimizations land, import indicators from Lean/StockIndicators; use profiling harness to validate parity and record performance deltas across all platforms.
8. **Documentation sync** – Update recipes, API docs, and wrapper samples alongside each major optimization phase; include NUMA/threading tuning guidance for high-core systems.

---
_This plan should be revisited after each major regeneration pass or indicator addition to keep the backlog relevant._
