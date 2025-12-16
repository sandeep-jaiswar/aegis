#!/usr/bin/env python3
"""
Aegis External Falsification Analysis Tool

This script analyzes benchmark results from all technology stacks and provides
an honest comparison of Aegis's performance claims.

Acceptance Criteria:
- Aegis wins on variance and predictability, not just averages
- Losses are explained, not hidden
- If Aegis doesn't win here, we revise — not rationalize
"""

import json
import sys
from pathlib import Path
from typing import Dict, List, Optional
from dataclasses import dataclass
from enum import Enum


class Technology(Enum):
    AEGIS = "Aegis"
    REACT = "React"
    CANVAS = "Canvas"
    QT = "Qt"
    IMGUI = "ImGui"


@dataclass
class BenchmarkResults:
    technology: str
    min_ns: int
    p50_ns: int
    p90_ns: int
    p95_ns: int
    p99_ns: int
    p99_9_ns: int
    max_ns: int
    mean_ns: int
    variance: float


def load_results(file_path: Path) -> Optional[BenchmarkResults]:
    """Load benchmark results from JSON file."""
    try:
        with open(file_path, 'r') as f:
            data = json.load(f)
        
        results = data['results']
        return BenchmarkResults(
            technology=data.get('technology', 'Unknown'),
            min_ns=results['min_ns'],
            p50_ns=results['p50_ns'],
            p90_ns=results['p90_ns'],
            p95_ns=results['p95_ns'],
            p99_ns=results['p99_ns'],
            p99_9_ns=results['p99_9_ns'],
            max_ns=results['max_ns'],
            mean_ns=results['mean_ns'],
            variance=results['variance']
        )
    except (FileNotFoundError, KeyError, json.JSONDecodeError) as e:
        print(f"Error loading {file_path}: {e}", file=sys.stderr)
        return None


def format_ns(ns: int) -> str:
    """Format nanoseconds in a human-readable way."""
    if ns < 1000:
        return f"{ns} ns"
    elif ns < 1_000_000:
        return f"{ns / 1000:.2f} μs"
    elif ns < 1_000_000_000:
        return f"{ns / 1_000_000:.2f} ms"
    else:
        return f"{ns / 1_000_000_000:.2f} s"


def calculate_improvement(baseline: float, current: float) -> str:
    """Calculate improvement percentage."""
    if baseline == 0:
        return "N/A"
    
    improvement = ((baseline - current) / baseline) * 100
    if improvement > 0:
        return f"✅ {improvement:.1f}% better"
    else:
        return f"❌ {abs(improvement):.1f}% worse"


def print_header():
    """Print analysis header."""
    print("╔══════════════════════════════════════════════════════════════╗")
    print("║  Aegis External Falsification Analysis                      ║")
    print("║  APP-001 Benchmark Comparison                                ║")
    print("╚══════════════════════════════════════════════════════════════╝")
    print()


def print_results_table(all_results: List[BenchmarkResults]):
    """Print comprehensive results table."""
    print("=" * 100)
    print("COMPREHENSIVE METRICS COMPARISON")
    print("=" * 100)
    print()
    
    # Header
    print(f"{'Metric':<15} ", end="")
    for result in all_results:
        print(f"{result.technology:<15} ", end="")
    print()
    print("-" * 100)
    
    # Metrics
    metrics = [
        ("Min", lambda r: r.min_ns),
        ("P50 (Median)", lambda r: r.p50_ns),
        ("P90", lambda r: r.p90_ns),
        ("P95", lambda r: r.p95_ns),
        ("P99", lambda r: r.p99_ns),
        ("P99.9", lambda r: r.p99_9_ns),
        ("Max", lambda r: r.max_ns),
        ("Mean", lambda r: r.mean_ns),
        ("Variance", lambda r: int(r.variance)),
    ]
    
    for metric_name, metric_fn in metrics:
        print(f"{metric_name:<15} ", end="")
        for result in all_results:
            value = metric_fn(result)
            print(f"{format_ns(value):<15} ", end="")
        print()
    
    print()


def analyze_variance(all_results: List[BenchmarkResults]):
    """Analyze variance - the key metric for Aegis."""
    print("=" * 100)
    print("VARIANCE ANALYSIS (Key Metric for Aegis)")
    print("=" * 100)
    print()
    
    aegis_result = next((r for r in all_results if r.technology.lower() == 'aegis'), None)
    
    if not aegis_result:
        print("❌ Aegis baseline results not found!")
        print("   Run: ./build/core/browser_comparison_demo")
        return
    
    print(f"Aegis Variance: {format_ns(int(aegis_result.variance))}²")
    print()
    
    print("Comparison vs Other Technologies:")
    print("-" * 100)
    
    for result in all_results:
        if result.technology.lower() == 'aegis':
            continue
        
        # Handle division by zero when Aegis variance is 0 (deterministic timing)
        if aegis_result.variance == 0:
            print(f"\n{result.technology}:")
            print(f"  Variance: {format_ns(int(result.variance))}²")
            if result.variance == 0:
                print(f"  vs Aegis: Both have zero variance (deterministic)")
            else:
                print(f"  vs Aegis: Cannot compare (Aegis using deterministic timing)")
                print(f"  ⚠️  Note: Aegis benchmark uses deterministic timestamp provider.")
                print(f"      Run with real timing to get actual variance comparison.")
        else:
            improvement_factor = result.variance / aegis_result.variance
            
            print(f"\n{result.technology}:")
            print(f"  Variance: {format_ns(int(result.variance))}²")
            print(f"  vs Aegis: {improvement_factor:.1f}x worse")
            
            if improvement_factor < 1:
                print(f"  ⚠️  WARNING: {result.technology} has BETTER variance than Aegis!")
                print(f"      This contradicts Aegis's claims. Investigation needed.")
            elif improvement_factor < 10:
                print(f"  ⚠️  {result.technology} is competitive with Aegis on variance.")
            elif improvement_factor < 100:
                print(f"  ✅  Aegis shows significant variance improvement.")
            else:
                print(f"  ✅  Aegis shows order-of-magnitude variance improvement!")
    
    print()


def analyze_p99(all_results: List[BenchmarkResults]):
    """Analyze P99 latency - predictability metric."""
    print("=" * 100)
    print("P99 LATENCY ANALYSIS (Predictability)")
    print("=" * 100)
    print()
    
    aegis_result = next((r for r in all_results if r.technology.lower() == 'aegis'), None)
    
    if not aegis_result:
        return
    
    print(f"Aegis P99: {format_ns(aegis_result.p99_ns)}")
    print()
    
    print("Comparison vs Other Technologies:")
    print("-" * 100)
    
    for result in all_results:
        if result.technology.lower() == 'aegis':
            continue
        
        improvement = calculate_improvement(result.p99_ns, aegis_result.p99_ns)
        
        print(f"\n{result.technology}:")
        print(f"  P99: {format_ns(result.p99_ns)}")
        print(f"  vs Aegis: {improvement}")
    
    print()


def honest_assessment(all_results: List[BenchmarkResults]):
    """Provide honest assessment of wins and losses."""
    print("=" * 100)
    print("HONEST ASSESSMENT")
    print("=" * 100)
    print()
    
    aegis_result = next((r for r in all_results if r.technology.lower() == 'aegis'), None)
    
    if not aegis_result:
        print("❌ Cannot perform assessment without Aegis baseline.")
        return
    
    print("Where Aegis Wins:")
    print("-" * 50)
    
    wins = []
    losses = []
    
    for result in all_results:
        if result.technology.lower() == 'aegis':
            continue
        
        # Check variance
        if aegis_result.variance < result.variance:
            wins.append(f"✅ Variance vs {result.technology}: "
                       f"{result.variance / aegis_result.variance:.1f}x better")
        else:
            losses.append(f"❌ Variance vs {result.technology}: "
                         f"{aegis_result.variance / result.variance:.1f}x worse")
        
        # Check P99
        if aegis_result.p99_ns < result.p99_ns:
            wins.append(f"✅ P99 vs {result.technology}: "
                       f"{result.p99_ns / aegis_result.p99_ns:.1f}x better")
        else:
            losses.append(f"❌ P99 vs {result.technology}: "
                         f"{aegis_result.p99_ns / result.p99_ns:.1f}x worse")
    
    if wins:
        for win in wins:
            print(win)
    else:
        print("⚠️  No clear wins found!")
    
    print()
    print("Where Aegis Loses (or is competitive):")
    print("-" * 50)
    
    if losses:
        for loss in losses:
            print(loss)
    else:
        print("✅ Aegis wins on all measured metrics!")
    
    print()
    print("Explanation:")
    print("-" * 50)
    print("""
Aegis's architectural advantages:
1. Explicit memory management (no GC pauses)
2. Deterministic execution (no JIT variance)
3. Frame allocator pattern (predictable allocation)
4. No framework overhead (direct execution)

Other technologies' trade-offs:
1. React: Virtual DOM reconciliation adds variance
2. Canvas: Browser rendering pipeline variance
3. Qt: Event loop and platform dependencies
4. All browser-based: JavaScript GC and JIT

Expected result: Aegis should have 10-100x better variance
                 due to deterministic design.
""")


def main():
    """Main analysis function."""
    print_header()
    
    # Look for result files
    current_dir = Path(__file__).parent
    result_files = {
        'Aegis': current_dir / 'aegis_results.json',
        'React': current_dir / 'react_results.json',
        'Canvas': current_dir / 'canvas_results.json',
        'Qt': current_dir / 'qt_results.json',
        'ImGui': current_dir / 'imgui_results.json',
    }
    
    all_results = []
    
    print("Loading benchmark results...")
    print()
    
    for tech, file_path in result_files.items():
        if file_path.exists():
            result = load_results(file_path)
            if result:
                all_results.append(result)
                print(f"✅ Loaded {tech} results")
        else:
            print(f"⚠️  {tech} results not found: {file_path}")
    
    print()
    
    if not all_results:
        print("❌ No benchmark results found!")
        print()
        print("Expected result files:")
        for tech, file_path in result_files.items():
            print(f"  - {file_path}")
        print()
        print("Please run benchmarks and export results to JSON files.")
        sys.exit(1)
    
    # Run analysis
    print_results_table(all_results)
    analyze_variance(all_results)
    analyze_p99(all_results)
    honest_assessment(all_results)
    
    print("=" * 100)
    print("CONCLUSION")
    print("=" * 100)
    print("""
If Aegis wins on variance and P99: ✅ Claims validated
If Aegis loses on variance or P99: ⚠️  Revise implementation, not rationalize

The goal is honest comparison, not marketing.
""")


if __name__ == "__main__":
    main()
