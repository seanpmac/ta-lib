#!/usr/bin/env python3
"""Generate a Markdown report comparing CPU and accelerated indicator performance."""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys
import time
from typing import Dict, Optional


def run_command(command, cwd: Optional[pathlib.Path] = None):
    start = time.perf_counter()
    proc = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        cwd=str(cwd) if cwd else None,
    )
    duration = time.perf_counter() - start
    return proc, duration


def _parse_section(section: str, patterns: Dict[str, str]):
    results: Dict[str, Optional[str]] = {}
    for key, pattern in patterns.items():
        match = re.search(pattern, section, re.MULTILINE)
        results[key] = match.group(1) if match else None
    return results


def parse_perf_output(output: str) -> Dict[str, Optional[float]]:
    metrics: Dict[str, Optional[float]] = {
        "samples": None,
        "period": None,
        "iterations": None,
        "sma_accel_enabled": None,
        "cpu_loop_ms": None,
        "ta_sma_ms": None,
        "ta_sma_max_diff": None,
        "ta_accel_ms": None,
        "ta_accel_max_diff": None,
        "sma_speedup": None,
        "plan_accel_enabled": None,
        "plan_accel_ms": None,
        "plan_cpu_ms": None,
        "plan_speedup": None,
        "plan_signals": None,
        "plan_max_diff": None,
    }

    sections = [sec.strip() for sec in re.split(r"\n\s*\n", output) if sec.strip()]
    sma_section = sections[0] if sections else ""
    plan_section = next((sec for sec in sections if sec.startswith("Multi-indicator plan benchmark")), "")

    sma_results = _parse_section(
        sma_section,
        {
            "samples": r"^Samples\s*:\s*(\d+)",
            "period": r"^Period\s*:\s*(\d+)",
            "iterations": r"^Iterations\s*:\s*(\d+)",
            "sma_accel_enabled": r"^Acceleration\s*:\s*(\S+)",
            "cpu_loop_ms": r"^CPU loop\s*:\s*([\d.]+)",
            "ta_sma_ms": r"^TA_SMA API\s*:\s*([\d.]+)",
            "ta_sma_max_diff": r"^TA_SMA API\s*:\s*[\d.]+\s*ms/iter \(max diff vs CPU ([^\)]+)\)",
            "ta_accel_ms": r"^TA_accel SMA\s*:\s*([\d.]+)",
            "ta_accel_max_diff": r"^TA_accel SMA\s*:\s*[\d.]+\s*ms/iter \(max diff vs CPU ([^\)]+)\)",
            "sma_speedup": r"^Speedup \(CPU/accel\):\s*([\d.]+)",
        },
    )

    plan_results = _parse_section(
        plan_section,
        {
            "plan_signals": r"^Signals computed:\s*(\d+)",
            "plan_accel_enabled": r"^Acceleration\s*:\s*(\S+)",
            "plan_accel_ms": r"^Accelerated plan:\s*([\d.]+)",
            "plan_cpu_ms": r"^CPU plan\s*:\s*([\d.]+)",
            "plan_speedup": r"^Speedup \(CPU/accel\):\s*([\d.]+)",
            "plan_max_diff": r"^Max diff vs CPU\s*:\s*([\d.eE+-]+)",
        },
    )

    def to_int(value: Optional[str]) -> Optional[int]:
        return int(value) if value is not None else None

    def to_float(value: Optional[str]) -> Optional[float]:
        try:
            return float(value) if value is not None else None
        except ValueError:
            return None

    metrics.update(
        {
            "samples": to_int(sma_results.get("samples")),
            "period": to_int(sma_results.get("period")),
            "iterations": to_int(sma_results.get("iterations")),
            "sma_accel_enabled": sma_results.get("sma_accel_enabled"),
            "cpu_loop_ms": to_float(sma_results.get("cpu_loop_ms")),
            "ta_sma_ms": to_float(sma_results.get("ta_sma_ms")),
            "ta_sma_max_diff": to_float(sma_results.get("ta_sma_max_diff")),
            "ta_accel_ms": to_float(sma_results.get("ta_accel_ms")),
            "ta_accel_max_diff": to_float(sma_results.get("ta_accel_max_diff")),
            "sma_speedup": to_float(sma_results.get("sma_speedup")),
            "plan_accel_enabled": plan_results.get("plan_accel_enabled"),
            "plan_accel_ms": to_float(plan_results.get("plan_accel_ms")),
            "plan_cpu_ms": to_float(plan_results.get("plan_cpu_ms")),
            "plan_speedup": to_float(plan_results.get("plan_speedup")),
            "plan_signals": to_int(plan_results.get("plan_signals")),
            "plan_max_diff": to_float(plan_results.get("plan_max_diff")),
        }
    )

    return metrics


def build_report(regtest_result, perf_result, metrics, regtest_duration, perf_duration) -> str:
    lines = []
    lines.append("# TA-Lib Acceleration Report\n")

    lines.append("## Regression Tests\n")
    lines.append("| Test | Result | Duration (s) | Notes |")
    lines.append("| --- | --- | --- | --- |")
    regtest_status = "PASS" if regtest_result.returncode == 0 else "FAIL"
    regtest_notes = "" if regtest_result.returncode == 0 else regtest_result.stderr.strip().replace("\n", " ")
    lines.append(f"| ta_regtest | {regtest_status} | {regtest_duration:.2f} | {regtest_notes or '-'} |")

    lines.append("\n## SMA Microbenchmark\n")
    if metrics.get("samples"):
        accel_state = metrics.get("sma_accel_enabled", "unknown")
        lines.append(f"Samples: **{metrics['samples']}** · Period: **{metrics.get('period', '-') }** · Iterations: **{metrics.get('iterations', '-')}** · Acceleration: **{accel_state}**\n")

    lines.append("| Path | Avg ms/iter | Speedup vs CPU | Max diff vs CPU |")
    lines.append("| --- | --- | --- | --- |")
    cpu_ms = metrics.get("cpu_loop_ms")
    if cpu_ms is not None:
        lines.append(f"| CPU loop | {cpu_ms:.3f} | 1.00x | 0 |")
    if metrics.get("ta_sma_ms") is not None:
        diff = metrics.get("ta_sma_max_diff")
        lines.append(f"| TA_SMA API | {metrics['ta_sma_ms']:.3f} | {cpu_ms / metrics['ta_sma_ms']:.2f}x | {diff or 0:.3g} |")
    if metrics.get("ta_accel_ms") is not None:
        diff = metrics.get("ta_accel_max_diff")
        speedup = cpu_ms / metrics['ta_accel_ms'] if cpu_ms and metrics['ta_accel_ms'] else None
        speedup_str = f"{speedup:.2f}x" if speedup else "-"
        lines.append(f"| TA_accel SMA | {metrics['ta_accel_ms']:.3f} | {speedup_str} | {diff or 0:.3g} |")

    lines.append("\n## Multi-Indicator Plan\n")
    plan_accel_state = metrics.get("plan_accel_enabled", "unknown")
    signals = metrics.get("plan_signals")
    if signals is not None:
        lines.append(f"Signals computed: **{signals}** · Acceleration: **{plan_accel_state}**\n")

    lines.append("| Plan Path | Avg ms/iter | Speedup vs CPU | Max diff vs CPU |")
    lines.append("| --- | --- | --- | --- |")
    if metrics.get("plan_cpu_ms") is not None:
        lines.append(f"| CPU plan | {metrics['plan_cpu_ms']:.3f} | 1.00x | 0 |")
    if metrics.get("plan_accel_ms") is not None:
        speedup = metrics.get("plan_speedup")
        diff = metrics.get("plan_max_diff")
        speedup_str = f"{speedup:.2f}x" if speedup else "-"
        lines.append(f"| Accelerated plan | {metrics['plan_accel_ms']:.3f} | {speedup_str} | {diff or 0:.3g} |")
    else:
        lines.append("| Accelerated plan | n/a | - | - |")

    lines.append("\n## Raw Outputs\n")
    lines.append("<details><summary>ta_regtest stdout</summary>\n\n````\n" + (regtest_result.stdout or "") + "\n````\n</details>")
    lines.append("<details><summary>ta_regtest stderr</summary>\n\n````\n" + (regtest_result.stderr or "") + "\n````\n</details>")
    lines.append("<details><summary>ta_perf output</summary>\n\n````\n" + (perf_result.stdout or "") + "\n````\n</details>")

    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate a performance and regression report for TA-Lib acceleration.")
    parser.add_argument("--regtest-path", type=pathlib.Path, default=pathlib.Path("build-metal/bin/ta_regtest"), help="Path to ta_regtest binary")
    parser.add_argument("--perf-path", type=pathlib.Path, default=pathlib.Path("build-metal/bin/ta_perf"), help="Path to ta_perf binary")
    parser.add_argument("--length", type=int, default=1_000_000, help="Sample length for ta_perf")
    parser.add_argument("--iterations", type=int, default=10, help="Iterations for ta_perf timing loops")
    parser.add_argument("--output", type=pathlib.Path, help="Optional Markdown file to write the report")
    parser.add_argument("--skip-regtest", action="store_true", help="Skip running the regression suite")
    parser.add_argument("--skip-perf", action="store_true", help="Skip running the performance benchmark")
    parser.add_argument("--cwd", type=pathlib.Path, default=pathlib.Path.cwd(), help="Working directory for commands")

    args = parser.parse_args()

    cwd = args.cwd
    regtest_result = subprocess.CompletedProcess(args=[str(args.regtest_path)], returncode=0, stdout="", stderr="")
    perf_result = subprocess.CompletedProcess(args=[str(args.perf_path)], returncode=0, stdout="", stderr="")
    regtest_duration = 0.0
    perf_duration = 0.0

    if not args.skip_regtest:
        if not args.regtest_path.exists():
            print(f"error: regtest binary not found at {args.regtest_path}", file=sys.stderr)
            return 1
        regtest_result, regtest_duration = run_command([str(args.regtest_path)], cwd)
    if not args.skip_perf:
        if not args.perf_path.exists():
            print(f"error: ta_perf binary not found at {args.perf_path}", file=sys.stderr)
            return 1
        perf_cmd = [str(args.perf_path), f"--length={args.length}", f"--iterations={args.iterations}"]
        perf_result, perf_duration = run_command(perf_cmd, cwd)

    metrics = parse_perf_output(perf_result.stdout) if perf_result.stdout else {}
    report = build_report(regtest_result, perf_result, metrics, regtest_duration, perf_duration)

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(report, encoding="utf-8")
        print(f"Report written to {args.output}")
    else:
        print(report)

    exit_code = 0
    if regtest_result.returncode != 0 and not args.skip_regtest:
        exit_code = regtest_result.returncode
    if perf_result.returncode != 0 and not args.skip_perf:
        exit_code = perf_result.returncode
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
