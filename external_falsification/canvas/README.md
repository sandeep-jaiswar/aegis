# APP-001 Canvas Benchmark

## Overview

This is the HTML5 Canvas implementation of the APP-001 benchmark workload for external falsification of Aegis's performance claims.

## Technology Stack

- **API**: HTML5 Canvas 2D
- **Runtime**: Chromium/Chrome browser

## Running the Benchmark

Simply open `canvas_benchmark.html` in Chrome/Chromium:

1. Close all other tabs and applications for accurate results
2. Open `canvas_benchmark.html` in your browser
3. Click "Start Benchmark"
4. Wait for completion (~10-15 seconds)
5. Review the metrics displayed
6. Click "Export Results" to save as JSON

## What This Measures

This implementation exercises the Canvas API with:

- **Rendering Operations**: Direct 2D drawing commands
- **GPU Rasterization**: Hardware-accelerated rendering
- **Canvas State Management**: Context save/restore, transformations
- **Memory Allocation**: ArrayBuffers and temporary objects
- **Browser Pipeline**: Compositing and display

## Expected Results

Compared to Aegis:

- ❌ **Higher variance**: GPU scheduling and browser pipeline introduce variance
- ❌ **Worse P99**: Tail latency affected by rendering overhead
- ⚠️ **Mixed average**: Canvas can be fast but unpredictable
- ❌ **Poor replay fidelity**: GPU timing is non-deterministic

## Visual Rendering

The benchmark actually renders the workload to canvas:

- **Allocation Rendering**: Each allocation is visualized as a colored rectangle
- **Operation Rendering**: Mathematical operations are drawn as lines
- **Frame Reset**: Canvas is cleared each frame

This exercises the full Canvas API stack, not just memory allocation.

## Files

- `canvas_benchmark.html` - Main HTML page with styling
- `canvas_benchmark.js` - Benchmark implementation
- `README.md` - This file

## Exporting Results

Results are exported in JSON format compatible with the analysis tool:

```json
{
  "technology": "Canvas",
  "timestamp": "2024-01-01T00:00:00.000Z",
  "config": {
    "warmupIterations": 10,
    "measuredIterations": 100,
    "allocationsPerFrame": 50,
    "operationsPerFrame": 100
  },
  "results": {
    "min_ns": 45000,
    "p50_ns": 72000,
    "p90_ns": 145000,
    "p95_ns": 195000,
    "p99_ns": 340000,
    "p99_9_ns": 480000,
    "max_ns": 720000,
    "mean_ns": 88000,
    "variance": 11250000000
  }
}
```

## Comparison with Aegis

After running this benchmark, compare with Aegis results:

```bash
# Run Aegis benchmark from project root
cd ../..
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/core/browser_comparison_demo
```

Use the analysis tool to compare:

```bash
cd external_falsification
python3 analyze_results.py
```
