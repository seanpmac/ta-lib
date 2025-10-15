#!/usr/bin/env python3
"""
Analyze indicator performance across multiple profiling runs
Identifies slowest indicators and optimization candidates
"""

import json
import sys
from pathlib import Path
from collections import defaultdict
from statistics import mean, median, stdev

def load_results(results_dir):
    """Load all JSON result files"""
    results = []
    results_path = Path(results_dir)
    
    for json_file in sorted(results_path.glob("*.json")):
        if json_file.name == "batch_results.json":
            continue
        
        try:
            with open(json_file, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                data = json.loads(content)
                results.append({
                    'file': json_file.stem,
                    'bars': data['metadata']['bar_count'],
                    'data': data
                })
        except (json.JSONDecodeError, UnicodeDecodeError, KeyError) as e:
            print(f"Warning: Could not parse {json_file}: {e}", file=sys.stderr)
            continue
    
    return results

def aggregate_indicator_stats(results):
    """Aggregate timing statistics for each indicator"""
    indicator_stats = defaultdict(lambda: {
        'times': [],
        'bars': [],
        'throughputs': []
    })
    
    for result in results:
        bars = result['bars']
        
        # Process serial results (primary timing source)
        for ind in result['data']['serial']['indicators']:
            name = ind['name']
            time_us = ind['total_time_us']
            throughput = ind['throughput_bars_per_sec']
            
            indicator_stats[name]['times'].append(time_us)
            indicator_stats[name]['bars'].append(bars)
            indicator_stats[name]['throughputs'].append(throughput)
    
    # Calculate statistics
    summary = []
    for name, stats in indicator_stats.items():
        times = stats['times']
        throughputs = stats['throughputs']
        
        summary.append({
            'name': name,
            'avg_time_us': mean(times),
            'median_time_us': median(times),
            'max_time_us': max(times),
            'stddev_time_us': stdev(times) if len(times) > 1 else 0,
            'avg_throughput': mean(throughputs),
            'runs': len(times)
        })
    
    return sorted(summary, key=lambda x: x['avg_time_us'], reverse=True)

def print_slowest_indicators(stats, top_n=20):
    """Print the slowest indicators"""
    print("=" * 100)
    print("SLOWEST INDICATORS - Top Candidates for SIMD Optimization")
    print("=" * 100)
    print(f"{'Rank':<6} {'Indicator':<20} {'Avg Time':<12} {'Max Time':<12} {'Throughput':<20} {'StdDev':<10}")
    print("-" * 100)
    
    for i, stat in enumerate(stats[:top_n], 1):
        avg_ms = stat['avg_time_us'] / 1000
        max_ms = stat['max_time_us'] / 1000
        throughput_mb = stat['avg_throughput'] / 1_000_000
        stddev_ms = stat['stddev_time_us'] / 1000
        
        print(f"{i:<6} {stat['name']:<20} {avg_ms:>10.3f}ms {max_ms:>10.3f}ms {throughput_mb:>10.2f} Mbars/s {stddev_ms:>8.3f}ms")
    
    print()

def print_fastest_indicators(stats, top_n=10):
    """Print the fastest indicators"""
    print("=" * 100)
    print("FASTEST INDICATORS - Already Optimized")
    print("=" * 100)
    print(f"{'Rank':<6} {'Indicator':<20} {'Avg Time':<12} {'Throughput':<20}")
    print("-" * 100)
    
    fastest = sorted(stats, key=lambda x: x['avg_time_us'])
    for i, stat in enumerate(fastest[:top_n], 1):
        avg_ms = stat['avg_time_us'] / 1000
        throughput_mb = stat['avg_throughput'] / 1_000_000
        
        print(f"{i:<6} {stat['name']:<20} {avg_ms:>10.3f}ms {throughput_mb:>10.2f} Mbars/s")
    
    print()

def categorize_indicators(stats):
    """Categorize indicators by performance characteristics"""
    categories = {
        'critical': [],      # > 500μs avg
        'high': [],          # 200-500μs
        'medium': [],        # 100-200μs
        'low': [],           # 50-100μs
        'optimized': []      # < 50μs
    }
    
    for stat in stats:
        time_us = stat['avg_time_us']
        
        if time_us > 500:
            categories['critical'].append(stat)
        elif time_us > 200:
            categories['high'].append(stat)
        elif time_us > 100:
            categories['medium'].append(stat)
        elif time_us > 50:
            categories['low'].append(stat)
        else:
            categories['optimized'].append(stat)
    
    return categories

def print_optimization_priorities(categories):
    """Print optimization priorities by category"""
    print("=" * 100)
    print("OPTIMIZATION PRIORITIES")
    print("=" * 100)
    print()
    
    priority_order = [
        ('critical', 'CRITICAL (>500μs) - Immediate SIMD/algorithmic optimization needed'),
        ('high', 'HIGH (200-500μs) - Significant SIMD benefits expected'),
        ('medium', 'MEDIUM (100-200μs) - Moderate SIMD benefits'),
        ('low', 'LOW (50-100μs) - Minor optimization opportunity'),
        ('optimized', 'OPTIMIZED (<50μs) - Already efficient')
    ]
    
    for key, description in priority_order:
        count = len(categories[key])
        if count > 0:
            print(f"{description:<70} Count: {count}")
            
            # Show top 3 in each category
            for stat in categories[key][:3]:
                print(f"  • {stat['name']:<20} {stat['avg_time_us']/1000:>8.3f}ms")
            
            if count > 3:
                print(f"  ... and {count-3} more")
            print()

def print_summary_stats(results, stats):
    """Print overall summary statistics"""
    total_bars = sum(r['bars'] for r in results)
    total_indicators = len(stats)
    
    print("=" * 100)
    print("OVERALL SUMMARY")
    print("=" * 100)
    print(f"Files analyzed:      {len(results)}")
    print(f"Total bars:          {total_bars:,}")
    print(f"Indicators tested:   {total_indicators}")
    print()
    
    # Calculate total compute time
    total_time_us = sum(s['avg_time_us'] for s in stats)
    print(f"Average serial time per file:  {total_time_us/1000:.2f}ms (all {total_indicators} indicators)")
    print()

def main():
    if len(sys.argv) > 1:
        results_dir = sys.argv[1]
    else:
        results_dir = "./profiling_results"
    
    print("\n")
    print("╔" + "═" * 98 + "╗")
    print("║" + " " * 30 + "TA-LIB PERFORMANCE ANALYSIS" + " " * 41 + "║")
    print("╚" + "═" * 98 + "╝")
    print()
    
    # Load and analyze
    results = load_results(results_dir)
    if not results:
        print(f"No results found in {results_dir}")
        return 1
    
    stats = aggregate_indicator_stats(results)
    
    # Print reports
    print_summary_stats(results, stats)
    print_slowest_indicators(stats, 25)
    print_fastest_indicators(stats, 15)
    
    categories = categorize_indicators(stats)
    print_optimization_priorities(categories)
    
    # Export to CSV for further analysis
    csv_file = Path(results_dir) / "indicator_analysis.csv"
    with open(csv_file, 'w') as f:
        f.write("indicator,avg_time_us,median_time_us,max_time_us,stddev_us,avg_throughput_bars_per_sec,runs\n")
        for stat in stats:
            f.write(f"{stat['name']},{stat['avg_time_us']:.2f},{stat['median_time_us']:.2f},"
                   f"{stat['max_time_us']:.2f},{stat['stddev_time_us']:.2f},"
                   f"{stat['avg_throughput']:.0f},{stat['runs']}\n")
    
    print(f"Detailed CSV exported to: {csv_file}")
    print()
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
