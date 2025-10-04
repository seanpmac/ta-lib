# TA-Lib Acceleration Report

## Regression Tests

| Test | Result | Duration (s) | Notes |
| --- | --- | --- | --- |
| ta_regtest | PASS | 2.88 | - |

## SMA Microbenchmark

Samples: **1000000** · Period: **30** · Iterations: **10** · Acceleration: **disabled/not**

| Path | Avg ms/iter | Speedup vs CPU | Max diff vs CPU |
| --- | --- | --- | --- |
| CPU loop | 1.869 | 1.00x | 0 |
| TA_SMA API | 1.867 | 1.00x | 0 |

## Multi-Indicator Plan

Signals computed: **5106380** · Acceleration: **disabled/not**

| Plan Path | Avg ms/iter | Speedup vs CPU | Max diff vs CPU |
| --- | --- | --- | --- |
| CPU plan | 21.902 | 1.00x | 0 |
| Accelerated plan | n/a | - | - |

## Raw Outputs

<details><summary>ta_regtest stdout</summary>

````

ta_regtest V0.6.4 (Oct  3 2025 22:41:21) - Regression Tests of TA-Lib code

Testing utility functions
Testing Abstract interface
Testing the TA functions
         MATH,VECTOR,DCPERIOD/PHASE,TRENDLINE/MODE: Testing....-\|/-\done.
                               All Moving Averages: Testing....-\|/-\|/-\|/-\|/-\|/-\|/-\|/-\|/-\|/-\|/-\|done.
                                 AROON,CORREL,BETA: Testing....-\|/-done.
                             CCI,WILLR,ULTOSC,NATR: Testing....-\|/-\|/done.
                                      BOP,AVGPRICE: Testing....-\|done.
                                           RSI,CMO: Testing....-\|/-done.
                                               IMI: Testing....-done.
                                        ACCEL PLAN: Testing....-done.
      MIN,MAX,MININDEX,MAXINDEX,MINMAX,MINMAXINDEX: Testing....-\|/-\|/-done.
                                            PO,APO: Testing....-\|/-\|/done.
                                 ADX,ADXR,DI,DM,DX: Testing....-\|/-\|/-\|/done.
                                        SAR,SAREXT: Testing....-done.
                             STOCH,STOCHF,STOCHRSI: Testing....-\|/-\|done.
                                      MFI,AD,ADOSC: Testing....-\|/-\|/done.
                                       PHASOR,SINE: Testing....-\|done.
                                              TRIX: Testing....-done.
                              MACD,MACDFIX,MACDEXT: Testing....-\|/done.
                         MOM,ROC,ROCP,ROCR,ROCR100: Testing....-\|/-done.
                                        TRANGE,ATR: Testing....-\|/done.
                                        STDDEV,VAR: Testing....-\|done.
                                            AVGDEV: Testing....-\done.
                                            BBANDS: Testing....-\|/-\|/-\|/-done.
                                  All Candlesticks: Testing....-\|/-\|/-\|/-\|/-\|/-\|/-\|/-\|/-\|/-\|/-\|/-\|/-\|/-\|/-\|/-\done.

Warning: Code profiling not supported for this platform.

* All tests succeeded. Enjoy the library. *

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
Acceleration   : disabled/not available
CPU loop       : 1.869 ms/iter
TA_SMA API     : 1.867 ms/iter (max diff vs CPU 0)

Multi-indicator plan benchmark (SMA/EMA/RSI/TRIMA/WMA)
Timeframes      : 5
Signals computed: 5106380
Acceleration    : disabled/not available
Accelerated plan: n/a
CPU plan        : 21.902 ms/iter
Max diff vs CPU : 0

````
</details>