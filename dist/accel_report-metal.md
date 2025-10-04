# TA-Lib Acceleration Report

## Regression Tests

| Test | Result | Duration (s) | Notes |
| --- | --- | --- | --- |
| ta_regtest | FAIL | 0.88 | - |

## SMA Microbenchmark

Samples: **1000000** · Period: **30** · Iterations: **10** · Acceleration: **enabled**

| Path | Avg ms/iter | Speedup vs CPU | Max diff vs CPU |
| --- | --- | --- | --- |
| CPU loop | 1.881 | 1.00x | 0 |
| TA_SMA API | 1.745 | 1.08x | 0.388 |
| TA_accel SMA | 1.513 | 1.24x | 0.388 |

## Multi-Indicator Plan

Signals computed: **5106380** · Acceleration: **enabled**

| Plan Path | Avg ms/iter | Speedup vs CPU | Max diff vs CPU |
| --- | --- | --- | --- |
| CPU plan | 22.077 | 1.00x | 0 |
| Accelerated plan | 25.910 | 0.85x | 6.01e+04 |

## Raw Outputs

<details><summary>ta_regtest stdout</summary>

````

ta_regtest V0.6.4 (Oct  4 2025 15:23:55) - Regression Tests of TA-Lib code

Testing utility functions
Testing Abstract interface
Testing the TA functions
         MATH,VECTOR,DCPERIOD/PHASE,TRENDLINE/MODE: Testing....-\|/-\done.
                               All Moving Averages: Testing....-\|/-\|/-\|/-\Fail: doRangeTestFixSize diff data for idx=0 (9.357750e+01,9.357748e+01)
Fail: doRangeTestFixSize (6,6,6,1,1)
Fail: doRangeTestFixSize refOutBeg,refOutNbElement (1,251)
Fail: Diff -1.6306e-05 %
Failed: For output #1 of 1
TA_MA Failed Test #24 (Code=161)

````
</details>
<details><summary>ta_regtest stderr</summary>

````

````
</details>
<details><summary>ta_perf output</summary>

````
TA-Lib SMA performance benchmark
Samples        : 1000000
Period         : 30
Iterations     : 10
Acceleration   : enabled
CPU loop       : 1.881 ms/iter
TA_SMA API     : 1.745 ms/iter (max diff vs CPU 0.388)
TA_accel SMA   : 1.513 ms/iter (max diff vs CPU 0.388)
Speedup (CPU/accel): 1.24x

Multi-indicator plan benchmark (SMA/EMA/RSI/TRIMA/WMA)
Timeframes      : 5
Signals computed: 5106380
Acceleration    : enabled
Accelerated plan: 25.910 ms/iter
CPU plan        : 22.077 ms/iter
Speedup (CPU/accel): 0.85x
Max diff vs CPU : 6.01e+04

````
</details>