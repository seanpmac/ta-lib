#!/usr/bin/env python3
import argparse
import glob
import json
import os
import pathlib
import subprocess
from statistics import median


def run_bench(
    ta_perf: str,
    data_dir: str,
    run_dir: str,
    indicators: str,
    iterations: int,
    mode: str,
) -> dict:
    os.makedirs(run_dir, exist_ok=True)
    csvs = sorted(glob.glob(os.path.join(data_dir, "*.csv")))
    results = {
        "run_dir": run_dir,
        "files": [],
        "datasets": [],
        "serial_total_us_list": [],
        "bars_total": 0,
    }
    for f in csvs:
        out = os.path.join(run_dir, pathlib.Path(f).with_suffix(".json").name)
        args = [
            ta_perf,
            "--input", f,
            "--mode", mode,
            "--iterations", str(iterations),
            "--output", out,
        ]
        # If indicators == 'all' (or empty), let ta_perf default to all by omitting the flag
        if indicators and indicators.lower() != "all":
            args.extend(["--indicators", indicators])
        subprocess.run(args, check=True)
        results["files"].append(out)
        # Quick summary read (reporter now emits forward slashes)
        try:
            with open(out, "r", encoding="utf-8") as fh:
                rep = json.load(fh)
            md = rep.get("metadata", {})
            serial = rep.get("serial")
            bar_count = int(md.get("bar_count", 0))
            results["bars_total"] += bar_count
            if serial:
                total_us = float(serial.get("total_time_us", 0.0))
                results["serial_total_us_list"].append(total_us)
                results["datasets"].append({
                    "name": pathlib.Path(md.get("input_file", f)).name,
                    "bars": bar_count,
                    "serial_total_us": total_us,
                })
        except Exception:
            pass
    return results


def print_summary(results: dict) -> None:
    files = results.get("files", [])
    datasets = results.get("datasets", [])
    serial_list = results.get("serial_total_us_list", [])
    bars_total = results.get("bars_total", 0)

    print("Benchmark completed")
    print(f"Run dir: {results.get('run_dir')}")
    print(f"Files processed: {len(files)}")
    print(f"Total bars: {bars_total:,}")
    if serial_list:
        total_s = sum(serial_list) / 1_000_000.0
        avg_s = (sum(serial_list) / len(serial_list)) / 1_000_000.0
        med_s = (median(serial_list)) / 1_000_000.0
        print(f"Serial total time (sum): {total_s:.3f}s")
        print(f"Serial total time (avg): {avg_s:.3f}s")
        print(f"Serial total time (median): {med_s:.3f}s")
    if datasets:
        # Show top 3 slowest by serial total
        slowest = sorted(datasets, key=lambda d: d.get("serial_total_us", 0.0), reverse=True)[:3]
        print("Top 3 slowest datasets (serial total):")
        for d in slowest:
            print(f"  - {d['name']}: {d['serial_total_us'] / 1_000_000.0:.3f}s ({d['bars']:,} bars)")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--ta-perf", default=os.path.join("build-win", "bin", "Release", "ta_perf.exe"))
    ap.add_argument("--data-dir", required=True)
    ap.add_argument("--run-dir", default=os.path.join("benchmarks", "latest", "eurusd"))
    ap.add_argument("--indicators", default="all")
    ap.add_argument("--iterations", type=int, default=5)
    ap.add_argument("--mode", default="serial")
    args = ap.parse_args()

    res = run_bench(
        ta_perf=args.ta_perf,
        data_dir=args.data_dir,
        run_dir=args.run_dir,
        indicators=args.indicators,
        iterations=args.iterations,
        mode=args.mode,
    )
    print_summary(res)


if __name__ == "__main__":
    main()
