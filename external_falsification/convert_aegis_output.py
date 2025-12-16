#!/usr/bin/env python3
"""
Convert Aegis C++ benchmark output to JSON format for comparison.

Usage:
    ./build/core/browser_comparison_demo | python3 convert_aegis_output.py > aegis_results.json
"""

import sys
import json
import re
from datetime import datetime


def parse_aegis_output(lines):
    """Parse Aegis benchmark output."""
    results = {}
    
    for line in lines:
        line = line.strip()
        
        # Parse metrics
        if 'Min:' in line:
            match = re.search(r'Min:\s+(\d+)\s+ns', line)
            if match:
                results['min_ns'] = int(match.group(1))
        
        elif 'P50:' in line:
            match = re.search(r'P50:\s+(\d+)\s+ns', line)
            if match:
                results['p50_ns'] = int(match.group(1))
        
        elif 'P90:' in line:
            match = re.search(r'P90:\s+(\d+)\s+ns', line)
            if match:
                results['p90_ns'] = int(match.group(1))
        
        elif 'P95:' in line:
            match = re.search(r'P95:\s+(\d+)\s+ns', line)
            if match:
                results['p95_ns'] = int(match.group(1))
        
        elif 'P99:' in line:
            match = re.search(r'P99:\s+(\d+)\s+ns', line)
            if match:
                results['p99_ns'] = int(match.group(1))
        
        elif 'P99.9:' in line:
            match = re.search(r'P99.9:\s+(\d+)\s+ns', line)
            if match:
                results['p99_9_ns'] = int(match.group(1))
        
        elif 'Max:' in line:
            match = re.search(r'Max:\s+(\d+)\s+ns', line)
            if match:
                results['max_ns'] = int(match.group(1))
        
        elif 'Mean:' in line:
            match = re.search(r'Mean:\s+(\d+)\s+ns', line)
            if match:
                results['mean_ns'] = int(match.group(1))
        
        elif 'Variance (est):' in line or 'Variance:' in line:
            match = re.search(r'Variance[^:]*:\s+(\d+)', line)
            if match:
                results['variance'] = int(match.group(1))
    
    return results


def main():
    # Read from stdin
    lines = sys.stdin.readlines()
    
    # Parse results
    results = parse_aegis_output(lines)
    
    if not results:
        print("Error: Could not parse Aegis output", file=sys.stderr)
        sys.exit(1)
    
    # Create JSON output
    output = {
        "technology": "Aegis",
        "timestamp": datetime.now().isoformat(),
        "config": {
            "warmupIterations": 10,
            "measuredIterations": 100,
            "allocationsPerFrame": 50,
            "operationsPerFrame": 100
        },
        "results": results
    }
    
    # Print JSON
    print(json.dumps(output, indent=2))


if __name__ == "__main__":
    main()
