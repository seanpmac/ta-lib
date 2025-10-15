# TA-Lib Performance Profiling Harness

A comprehensive benchmarking tool for measuring TA-Lib indicator performance across serial and parallel execution modes.

## Features

- **External data loading:** Reads OHLCV data from CSV files
- **Serial vs Parallel comparison:** Measures single-threaded and multi-threaded execution
- **Per-indicator metrics:** Individual timing for each indicator
- **Aggregate statistics:** Total runtime, throughput (bars/sec), speedup ratios
- **Structured output:** JSON and CSV export for baseline tracking and CI integration
- **Multi-pair batching:** Simulate real-world workloads with multiple currency pairs

## Usage

### Basic profiling (serial mode)
```bash
./ta_perf --input data/EURUSD.csv --mode serial --output results.json
```

### Parallel profiling with thread control
```bash
./ta_perf --input data/EURUSD.csv --mode parallel --threads 8 --output results.json
```

### Multi-pair batch profiling
```bash
./ta_perf --input-dir data/ --mode both --threads 128 --output batch_results.json
```

### Compare serial vs parallel
```bash
./ta_perf --input data/EURUSD.csv --mode both --threads auto --output comparison.json
```

## Input Format

CSV files with headers:
```csv
timestamp,open,high,low,close,volume
2024-01-01 00:00:00,1.1050,1.1055,1.1048,1.1052,1234
2024-01-01 00:01:00,1.1052,1.1060,1.1051,1.1058,2345
...
```

Required columns: `open`, `high`, `low`, `close`
Optional: `volume` (defaults to 0.0 if missing), `timestamp` (for logging)

## Output Format

### JSON output (`results.json`)
```json
{
  "metadata": {
    "timestamp": "2025-10-14T12:34:56Z",
    "platform": "Linux x86_64",
    "cpu_model": "AMD EPYC 7713",
    "cpu_cores": 128,
    "ta_lib_version": "0.6.4",
    "input_file": "data/EURUSD.csv",
    "bar_count": 50000
  },
  "serial": {
    "total_time_us": 125000.5,
    "indicators": [
      {
        "name": "SMA",
        "calls": 1,
        "total_time_us": 150.2,
        "avg_time_us": 150.2,
        "throughput_bars_per_sec": 332778
      },
      ...
    ]
  },
  "parallel": {
    "total_time_us": 18500.3,
    "thread_count": 8,
    "speedup": 6.76,
    "efficiency": 0.845,
    "indicators": [ ... ]
  }
}
```

### CSV output (`results.csv`)
```csv
indicator,mode,calls,total_time_us,avg_time_us,throughput_bars_per_sec
SMA,serial,1,150.2,150.2,332778
SMA,parallel,1,22.1,22.1,2262443
EMA,serial,1,180.5,180.5,277008
...
```

## Build

### CMake (integrated with main build)
```bash
cd build
cmake .. -DBUILD_DEV_TOOLS=ON
cmake --build . --target ta_perf
```

### Standalone
```bash
cd src/tools/ta_perf
mkdir build && cd build
cmake ..
make
```

## Command-line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--input FILE` | Path to single CSV file | Required if not using `--input-dir` |
| `--input-dir DIR` | Directory of CSV files (one per pair) | Optional |
| `--mode MODE` | Execution mode: `serial`, `parallel`, or `both` | `both` |
| `--threads N` | Thread count for parallel mode (`auto` = hardware_concurrency) | `auto` |
| `--max-threads N` | Cap thread count (useful for NUMA tuning) | 128 |
| `--output FILE` | Output file path (`.json` or `.csv` based on extension) | `stdout` |
| `--indicators LIST` | Comma-separated indicator names to test (default: all) | All |
| `--iterations N` | Repeat count for statistical stability | 1 |
| `--warmup N` | Warm-up iterations (not included in timings) | 1 |
| `--verbose` | Print detailed per-iteration logs | Off |

## Examples

### Baseline all indicators on 100k bars
```bash
./ta_perf --input bigdata.csv --mode both --output baseline_100k.json
```

### Test specific indicators
```bash
./ta_perf --input data.csv --indicators SMA,EMA,RSI,MACD --mode both
```

### NUMA-aware profiling on dual-socket EPYC
```bash
numactl --cpunodebind=0 --membind=0 ./ta_perf --input data.csv --threads 64 --output socket0.json
numactl --cpunodebind=1 --membind=1 ./ta_perf --input data.csv --threads 64 --output socket1.json
```

### CI regression check
```bash
./ta_perf --input testdata.csv --output current.json
python scripts/compare_baselines.py baseline.json current.json --threshold 5%
```

## Architecture

- **CSV Loader:** Parses input files, allocates aligned buffers
- **Serial Runner:** Executes indicators sequentially, measures individual timings
- **Parallel Runner:** Distributes indicators across thread pool, aggregates results
- **Timer:** Platform-specific high-resolution timing (QPC/Mach/CLOCK_MONOTONIC)
- **Reporter:** Formats and exports JSON/CSV with metadata

## Performance Tips

- Use `--warmup 3` for consistent results (warms caches)
- For multi-pair tests, ensure CSV files are pre-sorted and sized similarly
- Run with `nice -n -20` and disable CPU frequency scaling for reproducibility
- Redirect output to file to avoid terminal I/O overhead affecting timings

## Roadmap

- [ ] VTune/µProf/Instruments integration for cache/branch analysis
- [ ] Streaming (`_State`) vs batch mode comparison
- [ ] HTML report generation with charts
- [ ] Docker container for reproducible CI benchmarks
- [ ] Automated regression detection and alerting
