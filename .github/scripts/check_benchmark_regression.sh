#!/bin/bash
# Benchmark regression detection script
# This script compares current benchmark results against a baseline

set -e

# Check for required tools
if ! command -v awk &> /dev/null; then
    echo "❌ Error: 'awk' is required but not installed."
    exit 1
fi

BASELINE_FILE="${1:-baseline_benchmarks.txt}"
CURRENT_FILE="${2:-current_benchmarks.txt}"
THRESHOLD="${3:-0.05}"  # 5% regression threshold by default

echo "=== Aegis Benchmark Regression Detector ==="
echo "Baseline: $BASELINE_FILE"
echo "Current:  $CURRENT_FILE"
# Use awk for threshold percentage calculation
THRESHOLD_PCT=$(awk -v t="$THRESHOLD" 'BEGIN {printf "%.1f", t * 100}')
echo "Threshold: ${THRESHOLD_PCT}%"
echo ""

# Check if files exist
if [ ! -f "$CURRENT_FILE" ]; then
    echo "❌ Current benchmark file not found: $CURRENT_FILE"
    exit 1
fi

if [ ! -f "$BASELINE_FILE" ]; then
    echo "⚠️  Baseline file not found: $BASELINE_FILE"
    echo "This appears to be the first benchmark run."
    echo "Creating baseline from current results..."
    cp "$CURRENT_FILE" "$BASELINE_FILE"
    echo "✓ Baseline created successfully"
    exit 0
fi

# Extract P99 metrics from benchmark output
extract_p99() {
    local file=$1
    local benchmark_name=$2
    grep -A 20 "Benchmark: $benchmark_name" "$file" | grep "P99:" | awk '{print $2}' | head -1
}

# Function to compare metrics
compare_metric() {
    local baseline=$1
    local current=$2
    local threshold=$3
    
    if [ -z "$baseline" ] || [ -z "$current" ]; then
        echo "SKIP"
        return
    fi
    
    # Remove 'ns' suffix if present
    baseline=$(echo "$baseline" | sed 's/ns$//')
    current=$(echo "$current" | sed 's/ns$//')
    
    # Calculate percentage change using awk
    local change=$(awk -v c="$current" -v b="$baseline" 'BEGIN {printf "%.4f", (c - b) / b}')
    
    # Compare against threshold using awk
    local is_regression=$(awk -v ch="$change" -v th="$threshold" 'BEGIN {print (ch > th) ? 1 : 0}')
    local is_improvement=$(awk -v ch="$change" -v th="$threshold" 'BEGIN {print (ch < -th) ? 1 : 0}')
    
    if [ "$is_regression" = "1" ]; then
        echo "REGRESSED"
        return 1
    elif [ "$is_improvement" = "1" ]; then
        echo "IMPROVED"
        return 0
    else
        echo "UNCHANGED"
        return 0
    fi
}

# Check each benchmark
echo "Checking benchmarks:"
echo ""

REGRESSION_FOUND=0

# List of benchmarks to check
BENCHMARKS=(
    "frame_allocator_benchmark"
    "frame_lifecycle_benchmark"
)

for bench in "${BENCHMARKS[@]}"; do
    echo "  $bench:"
    
    baseline_p99=$(extract_p99 "$BASELINE_FILE" "$bench")
    current_p99=$(extract_p99 "$CURRENT_FILE" "$bench")
    
    if [ -z "$baseline_p99" ] || [ -z "$current_p99" ]; then
        echo "    Status: SKIP (metrics not found)"
        continue
    fi
    
    # Remove 'ns' suffix
    baseline_p99_clean=$(echo "$baseline_p99" | sed 's/ns$//')
    current_p99_clean=$(echo "$current_p99" | sed 's/ns$//')
    
    echo "    Baseline P99: $baseline_p99_clean ns"
    echo "    Current P99:  $current_p99_clean ns"
    
    status=$(compare_metric "$baseline_p99_clean" "$current_p99_clean" "$THRESHOLD") || result=$?
    
    # If result is not set, it means the function succeeded (returned 0)
    : ${result:=0}
    
    echo "    Status: $status"
    
    if [ $result -ne 0 ]; then
        REGRESSION_FOUND=1
        # Calculate change percentage using awk
        change=$(awk -v c="$current_p99_clean" -v b="$baseline_p99_clean" 'BEGIN {printf "%.2f", (c - b) / b * 100}')
        echo "    ⚠️  Performance regression detected: +${change}%"
    fi
    
    # Reset result for next iteration
    result=0
    
    echo ""
done

echo "=== Summary ==="
if [ $REGRESSION_FOUND -eq 0 ]; then
    echo "✓ No performance regressions detected"
    echo "All benchmarks are within acceptable thresholds"
    exit 0
else
    echo "❌ Performance regressions detected!"
    echo "Please investigate and optimize before merging"
    exit 1
fi
