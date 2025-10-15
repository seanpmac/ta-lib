# Quick Start: ta_perf Profiling Harness

## Build

From the ta-lib root directory:

```bash
# Configure with dev tools enabled (default)
cmake -B build -DBUILD_DEV_TOOLS=ON

# Build everything including ta_perf
cmake --build build

# ta_perf will be in build/bin/ and copied to bin/
./build/bin/ta_perf --help
```

## Prepare Your Data

Create a CSV file with OHLCV data:
- **Required columns:** `open`, `high`, `low`, `close`
- **Optional:** `volume`, `timestamp`
- **Format:** Header row, comma-separated values

Example (`my_data.csv`):
```csv
timestamp,open,high,low,close,volume
2024-01-01 00:00:00,1.1050,1.1055,1.1048,1.1052,1234
2024-01-01 00:01:00,1.1052,1.1060,1.1051,1.1058,2345
...
```

## Run Profiling

### Basic serial vs parallel comparison
```bash
./build/bin/ta_perf --input my_data.csv --mode both --output results.json
```

### Control thread count
```bash
# Use all available cores
./build/bin/ta_perf --input my_data.csv --mode parallel --threads auto

# Specific thread count
./build/bin/ta_perf --input my_data.csv --mode parallel --threads 64

# Cap maximum threads (useful for NUMA testing)
./build/bin/ta_perf --input my_data.csv --threads auto --max-threads 64
```

### Test specific indicators
```bash
./build/bin/ta_perf --input my_data.csv --indicators SMA,EMA,RSI,MACD --mode both
```

### Multiple iterations for statistical stability
```bash
./build/bin/ta_perf --input my_data.csv --mode both --iterations 10 --warmup 3 --output stable.json
```

### CSV output
```bash
./build/bin/ta_perf --input my_data.csv --mode both --output results.csv
```

## Output

### JSON format
```json
{
  "metadata": {
    "timestamp": "2025-10-14T12:34:56Z",
    "platform": "Linux x86_64",
    "cpu_model": "AMD EPYC 7713",
    "cpu_cores": 128,
    "bar_count": 50000
  },
  "serial": {
    "total_time_us": 125000.5,
    "indicators": [
      {
        "name": "SMA",
        "total_time_us": 150.2,
        "throughput_bars_per_sec": 332778
      },
      ...
    ]
  },
  "parallel": {
    "total_time_us": 18500.3,
    "thread_count": 64,
    "speedup": 6.76,
    "efficiency": 0.845,
    "indicators": [ ... ]
  }
}
```

## NUMA-Aware Profiling (EPYC)

Test each NUMA node separately:

```bash
# Socket 0
numactl --cpunodebind=0 --membind=0 ./build/bin/ta_perf \
  --input data.csv --threads 64 --output socket0.json

# Socket 1
numactl --cpunodebind=1 --membind=1 ./build/bin/ta_perf \
  --input data.csv --threads 64 --output socket1.json
```

## Baseline Tracking

1. Capture baseline:
```bash
./build/bin/ta_perf --input baseline_data.csv --mode both \
  --iterations 5 --output baselines/v0.6.4_epyc.json
```

2. After optimization, compare:
```bash
./build/bin/ta_perf --input baseline_data.csv --mode both \
  --iterations 5 --output results/optimized.json

# Use a comparison script (to be implemented)
python scripts/compare_baselines.py \
  baselines/v0.6.4_epyc.json \
  results/optimized.json \
  --threshold 5%
```

## Next Steps

- Expand `runner.c` to include all 102 indicators (currently ~15 as demonstration)
- Add streaming (`_State`) vs batch mode comparison
- Implement multi-file batch processing for `--input-dir`
- Create HTML report generator with charts
- Wire into CI for automated regression detection

See full documentation in `src/tools/ta_perf/README.md`
