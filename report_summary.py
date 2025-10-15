#!/usr/bin/env python3
"""Generate multi-file profiling summary report"""

import csv
import sys

def main():
    csv_file = "./profiling_results/batch_summary.csv" if len(sys.argv) < 2 else sys.argv[1]
    
    print("\n" + "=" * 100)
    print(" " * 30 + "TA-LIB MULTI-FILE PROFILING REPORT")
    print("=" * 100 + "\n")
    
    with open(csv_file) as f:
        reader = csv.DictReader(f)
        rows = list(reader)
    
    # Summary statistics
    total_bars = sum(int(r['bars']) for r in rows)
    avg_speedup = sum(float(r['speedup']) for r in rows) / len(rows)
    avg_efficiency = sum(float(r['efficiency']) for r in rows) / len(rows)
    total_serial = sum(float(r['serial_time_ms']) for r in rows)
    total_parallel = sum(float(r['parallel_time_ms']) for r in rows)
    
    print(f"{'Metric':<40} {'Value':>20}")
    print("-" * 60)
    print(f"{'Files Processed':<40} {len(rows):>20}")
    print(f"{'Total Bars Analyzed':<40} {total_bars:>20,}")
    print(f"{'Indicators per File':<40} {rows[0]['indicators']:>20}")
    print(f"{'Thread Count':<40} {rows[0]['threads']:>20}")
    print()
    print(f"{'Average Speedup (8 threads)':<40} {avg_speedup:>19.2f}x")
    print(f"{'Average Parallel Efficiency':<40} {avg_efficiency*100:>19.1f}%")
    print()
    print(f"{'Total Serial Time':<40} {total_serial:>18.2f}ms")
    print(f"{'Total Parallel Time':<40} {total_parallel:>18.2f}ms")
    print(f"{'Time Saved':<40} {total_serial - total_parallel:>18.2f}ms")
    print(f"{'Overall Speedup':<40} {total_serial/total_parallel:>19.2f}x")
    print()
    
    # Per-file breakdown
    print("\n" + "=" * 100)
    print("PER-FILE PERFORMANCE BREAKDOWN")
    print("=" * 100)
    print(f"{'File':<35} {'Bars':>8} {'Serial (ms)':>12} {'Parallel (ms)':>14} {'Speedup':>10} {'Efficiency':>12}")
    print("-" * 100)
    
    for row in rows:
        print(f"{row['file']:<35} {int(row['bars']):>8,} {float(row['serial_time_ms']):>12.2f} "
              f"{float(row['parallel_time_ms']):>14.2f} {float(row['speedup']):>9.2f}x "
              f"{float(row['efficiency'])*100:>11.1f}%")
    
    # Performance insights
    print("\n" + "=" * 100)
    print("PERFORMANCE INSIGHTS FOR EPYC 7713 (128 CORES)")
    print("=" * 100 + "\n")
    
    # Calculate projections
    epyc_cores = 128
    epyc_efficiency_estimate = 0.45  # Conservative estimate for 128 cores
    epyc_speedup = epyc_cores * epyc_efficiency_estimate
    
    avg_serial_per_file = total_serial / len(rows)
    epyc_time_per_file = avg_serial_per_file / epyc_speedup
    
    print(f"Current M1 Max (8 P-cores):")
    print(f"  • Average time per file:    {avg_serial_per_file:.2f}ms serial, {total_parallel/len(rows):.2f}ms parallel")
    print(f"  • Parallel efficiency:       {avg_efficiency*100:.1f}%")
    print(f"  • Speedup:                   {avg_speedup:.2f}x")
    print()
    
    print(f"Projected Dual EPYC 7713 (128 cores @ {epyc_efficiency_estimate*100:.0f}% efficiency):")
    print(f"  • Estimated speedup:         {epyc_speedup:.1f}x")
    print(f"  • Time per file:             {epyc_time_per_file:.2f}ms")
    print(f"  • Time for 13 files:         {epyc_time_per_file * 13:.2f}ms")
    print()
    
    # Multi-pair scenarios
    pairs_scenarios = [10, 28, 50, 100]
    print(f"Multi-Pair FX Scenarios (all {rows[0]['indicators']} indicators):")
    print(f"  {'Pairs':<10} {'M1 Serial':>15} {'M1 Parallel':>15} {'EPYC Projected':>18}")
    print("  " + "-" * 60)
    
    for pairs in pairs_scenarios:
        m1_serial = avg_serial_per_file * pairs
        m1_parallel = (total_parallel / len(rows)) * pairs
        epyc_projected = epyc_time_per_file * pairs
        
        print(f"  {pairs:<10} {m1_serial:>14.0f}ms {m1_parallel:>14.0f}ms {epyc_projected:>17.0f}ms")
    
    print()
    print("Note: 28 pairs = all major FX pairs, 50 pairs = extended coverage")
    print()
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
