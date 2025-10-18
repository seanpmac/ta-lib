# TA-Lib Performance Profiling Results

## Test Configuration
- **Dataset**: EURUSD second-by-second forex data (QuantConnect/Lean)
- **Data Points**: 24,388 bars
- **Iterations**: 100 runs per indicator
- **Build**: `-O3 -march=native` optimization

## Performance Results

### Top Performers (>1M samples/sec)
| Indicator | Throughput | Avg Time (µs) | Notes |
|-----------|------------|---------------|-------|
| MOM(10) | 6,253 K/s | 3.90 | Simple differencing |
| TRANGE | 3,011 K/s | 8.10 | Min/max of 3 values |
| ROC(10) | 1,555 K/s | 15.68 | Division operation |
| CDLDOJI | 982 K/s | 24.83 | Optimized pattern recognition |

### Fast Indicators (300-1000 K/s)
| Indicator | Throughput | Avg Time (µs) |
|-----------|------------|---------------|
| SMA(14) | 494 K/s | 49.35 |
| EMA(14) | 437 K/s | 55.84 |
| WMA(14) | 436 K/s | 55.94 |
| STDDEV(14) | 434 K/s | 56.13 |
| VAR(14) | 493 K/s | 49.50 |
| KAMA(14) | 412 K/s | 59.26 |
| T3(5,0.7) | 375 K/s | 64.96 |
| CDLHAMMER | 359 K/s | 67.96 |

### Medium Indicators (100-300 K/s)
| Indicator | Throughput | Avg Time (µs) |
|-----------|------------|---------------|
| ATR(14) | 174 K/s | 139.84 |
| NATR(14) | 175 K/s | 139.73 |
| RSI(14) | 168 K/s | 144.86 |
| BBANDS(20,2,2) | 185 K/s | 131.71 |
| DEMA(14) | 196 K/s | 124.18 |
| TEMA(14) | 136 K/s | 179.96 |
| MACD(12,26,9) | 135 K/s | 180.21 |
| ULTOSC(7,14,28) | 143 K/s | 170.23 |
| WILLR(14) | 126 K/s | 194.08 |
| ADX(14) | 121 K/s | 202.39 |
| ADXR(14) | 117 K/s | 207.91 |
| CCI(14) | 102 K/s | 238.74 |

### Slow Indicators (10-100 K/s)
| Indicator | Throughput | Avg Time (µs) | Notes |
|-----------|------------|---------------|-------|
| STOCH(14,3,3) | 81 K/s | 302.42 | Multiple nested EMAs |
| MIN(14) | 66 K/s | 370.41 | Deque overhead |
| MAX(14) | 66 K/s | 371.76 | Deque overhead |
| MINMAX(14) | 46 K/s | 528.93 | Double deque |

### Critical Performance Issues (<20 K/s)

#### Hilbert Transform Functions - **MAJOR BOTTLENECK**
| Indicator | Throughput | Avg Time (µs) | Slowdown vs SMA |
|-----------|------------|---------------|-----------------|
| HT_PHASOR | 19 K/s | 1,309.89 | **26x slower** |
| HT_DCPERIOD | 18 K/s | 1,344.21 | **27x slower** |
| HT_TRENDLINE | 15 K/s | 1,604.31 | **32x slower** |
| HT_DCPHASE | 4 K/s | 6,338.05 | **128x slower** |
| HT_SINE | 4 K/s | 6,707.51 | **136x slower** |
| HT_TRENDMODE | 4 K/s | 6,838.91 | **139x slower** |

#### Other Slow Functions
| Indicator | Throughput | Avg Time (µs) |
|-----------|------------|---------------|
| MAMA(0.5,0.05) | 15 K/s | 1,626.62 |

## Performance Analysis

### Category Averages
- **Overlap Studies**: ~300 K/s average (excluding MAMA)
- **Momentum Indicators**: ~140 K/s average
- **Volatility Indicators**: ~340 K/s average
- **Hilbert Transforms**: ~10 K/s average (**98% slower than other categories**)
- **Candlestick Patterns**: ~400 K/s average (optimized with vector helpers)

### Key Findings

1. **Hilbert Transform Performance Crisis**
   - HT functions are 100-400x slower than simple indicators
   - DCPhase/SINE/TRENDMODE are especially slow (4 K/s)
   - Circular buffer lookups and trigonometric operations are expensive
   - Complex state management with multiple price smoothers
   
2. **MIN/MAX Performance**
   - Slower than expected despite O(n) monotonic deque algorithm
   - Likely overhead from modulo operations in circular buffer
   - Could benefit from power-of-2 sizing and bitwise AND masking

3. **STOCH Performance**
   - Reasonable for complexity (multiple EMA/SMA computations)
   - Could benefit from fused computation to reduce passes

4. **Candlestick Patterns**
   - Recent vector optimizations show good results
   - CDLDOJI at 982 K/s demonstrates effectiveness
   - More complex patterns (MORNINGSTAR, EVENINGSTAR) at ~135 K/s

## Optimization Recommendations

### Priority 1: Hilbert Transform Functions (Critical)
**Estimated Impact**: 10-20x speedup potential

1. **Cache Trigonometric Values**
   - Pre-compute `sin(i*2π/DCPeriodInt)` and `cos(i*2π/DCPeriodInt)`
   - Use lookup tables for common periods (6-50)
   
2. **Simplify Circular Buffer Access**
   - Replace modulo with power-of-2 masking where possible
   - Unroll small buffer loops
   
3. **Vectorize Core Loops**
   - The DCPeriod calculation loop is SIMD-friendly
   - Apply `#pragma omp simd` to accumulation loops
   
4. **Reduce State Complexity**
   - Combine redundant smoothing operations
   - Minimize branching in hot loops

### Priority 2: MIN/MAX Functions
**Estimated Impact**: 2-3x speedup

1. **Optimize Deque Implementation**
   - Use power-of-2 capacity for bitwise AND instead of modulo
   - Consider cache-friendly data layout
   
2. **Specialize for Common Periods**
   - Small periods (< 16) could use simple scan
   - Large periods benefit from current deque approach

### Priority 3: STOCH and Complex Momentum Indicators
**Estimated Impact**: 1.5-2x speedup

1. **Fused Computation**
   - Combine multiple MA calculations into single pass
   - Reduce intermediate buffer allocations
   
2. **Optimize Nested Calls**
   - Inline critical EMA/SMA operations
   - Share state between related calculations

### Priority 4: General Optimizations

1. **SIMD Enhancement**
   - Apply vectorization hints to more functions
   - Use restrict qualifiers consistently (now debugged)
   
2. **Cache Optimization**
   - Improve data locality in sliding window functions
   - Align buffers to cache line boundaries
   
3. **Branch Prediction**
   - Minimize conditionals in hot loops
   - Use likely/unlikely hints for error paths

## Next Steps

1. **Implement Hilbert Transform optimizations** (highest impact)
2. **Profile again** to measure improvements
3. **Apply learnings to other slow functions**
4. **Document optimization patterns** for future work

## Test Reproducibility

```bash
# Compile profiling tool
cd /Users/Sean/GitHub/ta-lib/tools
gcc -O3 -march=native -I../include ../build/libta-lib.a -o profile_forex profile_forex.c -lm

# Run profiling
./profile_forex /Users/Sean/GitHub/QuantConnect/Lean/Data/forex/oanda/second/eurusd/20140508_eurusd_second_quote.csv
```

## Historical Context

This profiling follows:
- Candlestick vector optimization (completed)
- Hilbert atan2 refactoring for numerical stability (completed)
- Regression test validation (all tests pass)

The next phase focuses on performance rather than correctness.
