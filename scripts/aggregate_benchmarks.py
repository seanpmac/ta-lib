#!/usr/bin/env python3
"""Aggregate ta_perf benchmark JSON reports into a Markdown summary.

Usage:
    python scripts/aggregate_benchmarks.py --run-dir benchmarks/20251015T050513Z/eurusd \
        --output benchmarks/20251015T050513Z/eurusd/report.md

The script scans for `*.json` files produced by `ta_perf --mode both`,
computes per-dataset and aggregate statistics (serial vs parallel), and
emits a Markdown report that can be diffed across benchmark runs.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import subprocess
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple


@dataclass
class DatasetSummary:
    name: str
    bar_count: int
    serial_total_us: Optional[float]
    parallel_total_us: Optional[float]
    thread_count: Optional[int]

    @property
    def speedup(self) -> Optional[float]:
        if self.serial_total_us and self.parallel_total_us and self.parallel_total_us > 0:
            return self.serial_total_us / self.parallel_total_us
        return None

    @property
    def efficiency(self) -> Optional[float]:
        if self.speedup and self.thread_count and self.thread_count > 0:
            return self.speedup / self.thread_count
        return None


@dataclass
class IndicatorAggregate:
    name: str
    serial_time_us: float = 0.0
    parallel_time_us: float = 0.0
    calls: int = 0
    datasets_seen: int = 0

    @property
    def avg_serial_us(self) -> float:
        return self.serial_time_us / self.datasets_seen if self.datasets_seen else 0.0

    @property
    def avg_parallel_us(self) -> float:
        return self.parallel_time_us / self.datasets_seen if self.datasets_seen else 0.0

    @property
    def speedup(self) -> Optional[float]:
        if self.avg_parallel_us > 0:
            return self.avg_serial_us / self.avg_parallel_us
        return None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--run-dir",
        required=True,
        type=Path,
        help="Directory containing ta_perf JSON reports",
    )
    parser.add_argument(
        "--output",
        required=True,
        type=Path,
        help="Path to write Markdown summary",
    )
    parser.add_argument(
        "--summary-json",
        type=Path,
        help="Optional path for summary JSON output (default: <run-dir>/summary.json)",
    )
    parser.add_argument(
        "--indicator-csv",
        type=Path,
        help="Optional path for indicator CSV output (default: <run-dir>/indicator_metrics.csv)",
    )
    parser.add_argument(
        "--top-indicators",
        type=int,
        default=10,
        help="Number of indicators to list in top speedup sections (default: 10)",
    )
    return parser.parse_args()


def load_reports(run_dir: Path) -> List[Dict]:
    reports: List[Dict] = []
    if not run_dir.exists():
        raise FileNotFoundError(f"Run directory not found: {run_dir}")

    for path in sorted(run_dir.glob("*.json")):
        with path.open("r", encoding="utf-8") as f:
            try:
                reports.append(json.load(f))
            except json.JSONDecodeError as exc:
                raise ValueError(f"Failed to parse {path}: {exc}") from exc
    if not reports:
        raise ValueError(f"No JSON reports found in {run_dir}")
    return reports


def extract_dataset_summaries(reports: Iterable[Dict]) -> Tuple[List[DatasetSummary], Dict[str, Optional[int]]]:
    summaries_map: Dict[str, DatasetSummary] = {}
    thread_counts: Dict[str, Optional[int]] = {}
    for report in reports:
        metadata = report.get("metadata", {})
        dataset_name = Path(metadata.get("input_file", "unknown")).name
        bar_count = int(metadata.get("bar_count", 0))

        serial = report.get("serial")
        parallel = report.get("parallel")

        serial_total = serial.get("total_time_us") if serial else None
        parallel_total = parallel.get("total_time_us") if parallel else None
        thread_count = parallel.get("thread_count") if parallel else None

        summaries_map[dataset_name] = DatasetSummary(
            name=dataset_name,
            bar_count=bar_count,
            serial_total_us=serial_total,
            parallel_total_us=parallel_total,
            thread_count=thread_count,
        )
        thread_counts[dataset_name] = thread_count

    summaries = [summaries_map[name] for name in sorted(summaries_map.keys())]
    return summaries, thread_counts


def aggregate_indicators(reports: Iterable[Dict]) -> Dict[str, IndicatorAggregate]:
    aggregates: Dict[str, IndicatorAggregate] = {}

    for report in reports:
        serial_lookup = {ind["name"]: ind for ind in report.get("serial", {}).get("indicators", [])}
        parallel_lookup = {ind["name"]: ind for ind in report.get("parallel", {}).get("indicators", [])}

        indicator_names = set(serial_lookup) | set(parallel_lookup)
        for name in indicator_names:
            entry = aggregates.setdefault(name, IndicatorAggregate(name=name))

            serial_ind = serial_lookup.get(name)
            if serial_ind:
                entry.serial_time_us += float(serial_ind.get("total_time_us", 0.0))
                entry.calls += int(serial_ind.get("calls", 0))

            parallel_ind = parallel_lookup.get(name)
            if parallel_ind:
                entry.parallel_time_us += float(parallel_ind.get("total_time_us", 0.0))

            if serial_ind or parallel_ind:
                entry.datasets_seen += 1

    return aggregates


def fmt(value: Optional[float], precision: int = 2, default: str = "–") -> str:
    if value is None:
        return default
    return f"{value:.{precision}f}"


def percentile(values: List[float], p: float) -> Optional[float]:
    if not values:
        return None
    ordered = sorted(values)
    k = (len(ordered) - 1) * (p / 100.0)
    f = math.floor(k)
    c = math.ceil(k)
    if f == c:
        return ordered[int(k)]
    return ordered[f] + (ordered[c] - ordered[f]) * (k - f)


def load_category_mapping(run_dir: Path) -> Dict[str, List[str]]:
    mapping_path = Path(__file__).with_name("indicator_categories.json")
    if mapping_path.exists():
        with mapping_path.open("r", encoding="utf-8") as f:
            return json.load(f)
    return {}


def compute_category_rollups(
    indicators: Dict[str, IndicatorAggregate],
    categories: Dict[str, List[str]],
) -> List[Dict[str, Optional[float]]]:
    rollups: List[Dict[str, Optional[float]]] = []
    for category, names in categories.items():
        members = [indicators[name] for name in names if name in indicators]
        if not members:
            continue
        serial_total = sum(m.serial_time_us for m in members)
        parallel_total = sum(m.parallel_time_us for m in members)
        speedup = (
            (serial_total / parallel_total)
            if parallel_total and parallel_total > 0
            else None
        )
        rollups.append(
            {
                "category": category,
                "serial_total_us": serial_total,
                "parallel_total_us": parallel_total,
                "speedup": speedup,
                "indicator_count": len(members),
            }
        )
    rollups.sort(key=lambda item: item["category"].lower())
    return rollups


def gather_environment_metadata(
    reports: List[Dict],
    thread_counts: Dict[str, Optional[int]],
) -> Dict[str, Optional[str]]:
    first_report = reports[0] if reports else {}
    metadata = first_report.get("metadata", {})
    thread_values = [tc for tc in thread_counts.values() if tc]
    avg_threads = sum(thread_values) / len(thread_values) if thread_values else None

    try:
        git_commit = (
            subprocess.check_output(["git", "rev-parse", "HEAD"], text=True)
            .strip()
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        git_commit = None

    return {
        "run_metadata": metadata,
        "average_thread_count": avg_threads,
        "git_commit": git_commit,
    }


def write_summary_json(
    summary_path: Path,
    run_name: str,
    summaries: List[DatasetSummary],
    totals: Dict[str, Optional[float]],
    indicators: Dict[str, IndicatorAggregate],
    categories: List[Dict[str, Optional[float]]],
    environment: Dict[str, Optional[str]],
) -> None:
    indicator_snapshot = {
        name: {
            "avg_serial_us": agg.avg_serial_us,
            "avg_parallel_us": agg.avg_parallel_us,
            "speedup": agg.speedup,
            "datasets_seen": agg.datasets_seen,
            "calls": agg.calls,
        }
        for name, agg in indicators.items()
    }

    summary_data = {
        "run": run_name,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "totals": totals,
        "datasets": [
            {
                "name": summary.name,
                "bar_count": summary.bar_count,
                "serial_total_us": summary.serial_total_us,
                "parallel_total_us": summary.parallel_total_us,
                "speedup": summary.speedup,
                "efficiency": summary.efficiency,
            }
            for summary in summaries
        ],
        "categories": categories,
        "indicators": indicator_snapshot,
        "environment": environment,
    }

    with summary_path.open("w", encoding="utf-8") as f:
        json.dump(summary_data, f, indent=2)


def write_indicator_csv(csv_path: Path, indicators: Dict[str, IndicatorAggregate]) -> None:
    with csv_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow([
            "indicator",
            "avg_serial_us",
            "avg_parallel_us",
            "speedup",
            "datasets_seen",
            "calls",
        ])
        for name in sorted(indicators.keys()):
            agg = indicators[name]
            writer.writerow([
                name,
                f"{agg.avg_serial_us:.6f}",
                f"{agg.avg_parallel_us:.6f}",
                f"{agg.speedup:.6f}" if agg.speedup else "",
                agg.datasets_seen,
                agg.calls,
            ])


def write_markdown(
    summaries: List[DatasetSummary],
    indicators: Dict[str, IndicatorAggregate],
    output_path: Path,
    run_dir: Path,
    top_n: int,
    totals: Dict[str, Optional[float]],
    category_rollups: List[Dict[str, Optional[float]]],
    environment: Dict[str, Optional[str]],
) -> None:
    total_serial = [s.serial_total_us for s in summaries if s.serial_total_us]
    total_parallel = [s.parallel_total_us for s in summaries if s.parallel_total_us]

    sum_serial = totals.get("serial_total_us")
    sum_parallel = totals.get("parallel_total_us")
    avg_serial = totals.get("serial_avg_us")
    avg_parallel = totals.get("parallel_avg_us")
    avg_speedup = totals.get("average_speedup")
    median_serial = totals.get("serial_median_us")
    median_parallel = totals.get("parallel_median_us")
    p95_serial = totals.get("serial_p95_us")
    p95_parallel = totals.get("parallel_p95_us")

    run_name = run_dir.name

    with output_path.open("w", encoding="utf-8") as out:
        out.write(f"# Benchmark Summary – {run_name}\n\n")

        out.write("## Dataset Overview\n\n")
        out.write("| Dataset | Bars | Serial Total (s) | Parallel Total (s) | Speedup | Efficiency |\n")
        out.write("| --- | ---: | ---: | ---: | ---: | ---: |\n")
        for summary in summaries:
            serial_sec = summary.serial_total_us / 1_000_000 if summary.serial_total_us else None
            parallel_sec = (
                summary.parallel_total_us / 1_000_000 if summary.parallel_total_us else None
            )
            out.write(
                f"| {summary.name} | {summary.bar_count:,} | {fmt(serial_sec, 3)} | "
                f"{fmt(parallel_sec, 3)} | {fmt(summary.speedup, 2)} | {fmt(summary.efficiency, 3)} |\n"
            )

        out.write("\n## Aggregate Metrics\n\n")
        out.write("- **[Total Serial Time]** {} s\n".format(
            fmt(sum_serial / 1_000_000 if sum_serial else None, 3)
        ))
        out.write("- **[Total Parallel Time]** {} s\n".format(
            fmt(sum_parallel / 1_000_000 if sum_parallel else None, 3)
        ))
        out.write("- **[Average Serial Total]** {} s\n".format(
            fmt(avg_serial / 1_000_000 if avg_serial else None, 3)
        ))
        out.write("- **[Average Parallel Total]** {} s\n".format(
            fmt(avg_parallel / 1_000_000 if avg_parallel else None, 3)
        ))
        out.write("- **[Median Serial Total]** {} s\n".format(
            fmt(median_serial / 1_000_000 if median_serial else None, 3)
        ))
        out.write("- **[Median Parallel Total]** {} s\n".format(
            fmt(median_parallel / 1_000_000 if median_parallel else None, 3)
        ))
        out.write("- **[95th Percentile Serial]** {} s\n".format(
            fmt(p95_serial / 1_000_000 if p95_serial else None, 3)
        ))
        out.write("- **[95th Percentile Parallel]** {} s\n".format(
            fmt(p95_parallel / 1_000_000 if p95_parallel else None, 3)
        ))
        out.write("- **[Average Speedup]** {}\n".format(fmt(avg_speedup, 2)))
        out.write("- **[Datasets Benchmarked]** {}\n".format(len(summaries)))

        out.write("\n## Environment\n\n")
        run_metadata = environment.get("run_metadata", {}) if environment else {}
        out.write("- **[ta_perf Timestamp]** {}\n".format(run_metadata.get("timestamp", "–")))
        out.write("- **[Platform]** {}\n".format(run_metadata.get("platform", "–")))
        out.write("- **[CPU Model]** {}\n".format(run_metadata.get("cpu_model", "–")))
        out.write("- **[CPU Cores]** {}\n".format(run_metadata.get("cpu_cores", "–")))
        out.write("- **[Input File Example]** {}\n".format(run_metadata.get("input_file", "–")))
        out.write("- **[Average Threads]** {}\n".format(fmt(environment.get("average_thread_count"), 2)))
        out.write("- **[Git Commit]** {}\n".format(environment.get("git_commit", "–")))

        sorted_indicators = sorted(
            indicators.values(),
            key=lambda ind: (ind.speedup or 0.0),
            reverse=True,
        )

        out.write("\n## Top Indicators by Speedup\n\n")
        out.write(
            "| Indicator | Avg Serial (µs) | Avg Parallel (µs) | Speedup | Datasets | Calls |\n"
        )
        out.write("| --- | ---: | ---: | ---: | ---: | ---: |\n")
        for indicator in sorted_indicators[:top_n]:
            out.write(
                f"| {indicator.name} | {fmt(indicator.avg_serial_us, 2)} | "
                f"{fmt(indicator.avg_parallel_us, 2)} | {fmt(indicator.speedup, 2)} | "
                f"{indicator.datasets_seen} | {indicator.calls} |\n"
            )

        slowest = sorted(
            indicators.values(),
            key=lambda ind: ind.avg_parallel_us,
            reverse=True,
        )

        out.write("\n## Slowest Indicators (Parallel Average)\n\n")
        out.write("| Indicator | Avg Parallel (µs) | Avg Serial (µs) | Speedup | Datasets |\n")
        out.write("| --- | ---: | ---: | ---: | ---: |\n")
        for indicator in slowest[:top_n]:
            out.write(
                f"| {indicator.name} | {fmt(indicator.avg_parallel_us, 2)} | "
                f"{fmt(indicator.avg_serial_us, 2)} | {fmt(indicator.speedup, 2)} | "
                f"{indicator.datasets_seen} |\n"
            )

        all_indicators = sorted(indicators.values(), key=lambda ind: ind.name)

        out.write("\n## All Indicators\n\n")
        out.write(
            "| Indicator | Avg Serial (µs) | Avg Parallel (µs) | Speedup | Datasets | Calls |\n"
        )
        out.write("| --- | ---: | ---: | ---: | ---: | ---: |\n")
        for indicator in all_indicators:
            out.write(
                f"| {indicator.name} | {fmt(indicator.avg_serial_us, 2)} | "
                f"{fmt(indicator.avg_parallel_us, 2)} | {fmt(indicator.speedup, 2)} | "
                f"{indicator.datasets_seen} | {indicator.calls} |\n"
            )

        if category_rollups:
            out.write("\n## Category Rollups\n\n")
            out.write(
                "| Category | Indicators | Serial Total (ms) | Parallel Total (ms) | Speedup |\n"
            )
            out.write("| --- | ---: | ---: | ---: | ---: |\n")
            for category in category_rollups:
                serial_ms = (
                    category["serial_total_us"] / 1000.0
                    if category["serial_total_us"]
                    else None
                )
                parallel_ms = (
                    category["parallel_total_us"] / 1000.0
                    if category["parallel_total_us"]
                    else None
                )
                out.write(
                    f"| {category['category']} | {category['indicator_count']} | "
                    f"{fmt(serial_ms, 2)} | {fmt(parallel_ms, 2)} | "
                    f"{fmt(category['speedup'], 2)} |\n"
                )

        out.write("\n---\n")
        out.write(
            "_Generated by `scripts/aggregate_benchmarks.py`. Rerun after future `ta_perf` "
            "benchmarks to compare results._\n"
        )


def main() -> None:
    args = parse_args()
    reports = load_reports(args.run_dir)
    summaries, thread_counts = extract_dataset_summaries(reports)
    indicators = aggregate_indicators(reports)

    total_serial = [s.serial_total_us for s in summaries if s.serial_total_us]
    total_parallel = [s.parallel_total_us for s in summaries if s.parallel_total_us]
    totals = {
        "serial_total_us": sum(total_serial) if total_serial else None,
        "parallel_total_us": sum(total_parallel) if total_parallel else None,
        "serial_avg_us": (sum(total_serial) / len(total_serial)) if total_serial else None,
        "parallel_avg_us": (sum(total_parallel) / len(total_parallel)) if total_parallel else None,
        "serial_median_us": percentile(total_serial, 50) if total_serial else None,
        "parallel_median_us": percentile(total_parallel, 50) if total_parallel else None,
        "serial_p95_us": percentile(total_serial, 95) if total_serial else None,
        "parallel_p95_us": percentile(total_parallel, 95) if total_parallel else None,
        "average_speedup": (
            (sum(total_serial) / len(total_serial))
            / (sum(total_parallel) / len(total_parallel))
            if total_serial and total_parallel and sum(total_parallel) > 0
            else None
        ),
    }

    categories = load_category_mapping(args.run_dir)
    category_rollups = compute_category_rollups(indicators, categories)
    environment = gather_environment_metadata(reports, thread_counts)

    summary_json_path = args.summary_json or args.run_dir / "summary.json"
    write_summary_json(
        summary_json_path,
        args.run_dir.name,
        summaries,
        totals,
        indicators,
        category_rollups,
        environment,
    )

    indicator_csv_path = args.indicator_csv or args.run_dir / "indicator_metrics.csv"
    write_indicator_csv(indicator_csv_path, indicators)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    write_markdown(
        summaries,
        indicators,
        args.output,
        args.run_dir,
        args.top_indicators,
        totals,
        category_rollups,
        environment,
    )


if __name__ == "__main__":
    main()
