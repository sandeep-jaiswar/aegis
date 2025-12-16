# External Falsification - APP-001 Benchmark

## Overview

This directory contains reimplementations of the APP-001 benchmark workload using different technology stacks to validate Aegis's claims about performance variance and predictability.

## Purpose

The purpose of external falsification is to **deliberately challenge** Aegis's architectural claims by implementing the same workload in alternative technologies:

1. **Chromium + React**: Modern web framework with virtual DOM
2. **Chromium + Canvas**: Direct 2D rendering API
3. **Native Qt**: Cross-platform native GUI framework
4. **ImGui** (optional): Immediate mode GUI library

## The Workload (APP-001)

APP-001 is a standardized benchmark workload that simulates typical UI application behavior:

- **Frame Allocations**: 50 allocations per frame with sizes [64, 128, 256, 512, 1024] bytes
- **Computational Operations**: 100 mathematical operations per frame (i² + i)
- **Frame Lifecycle**: Reset and repeat for each frame

## Metrics

All implementations measure identical metrics:

1. **P99 Frame Time**: 99th percentile frame execution time
2. **Frame Variance**: Statistical variance of frame times
3. **Memory Stability**: Memory allocation patterns and GC behavior
4. **Replay Fidelity**: Ability to reproduce exact results

## Acceptance Criteria

✅ **Aegis wins on variance and predictability**, not just averages
✅ **Losses are explained**, not hidden
⚠️ **If Aegis doesn't win here, we revise — not rationalize**

## Running the Benchmarks

### 1. Aegis (Baseline)

```bash
# Build Aegis
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run Aegis benchmark
./build/core/browser_comparison_demo
```

### 2. Chromium + React

```bash
cd external_falsification/react
npm install
npm run build
npm start
# Open http://localhost:3000 and click "Run Benchmark"
```

### 3. Chromium + Canvas

```bash
cd external_falsification/canvas
# Open canvas_benchmark.html in Chrome/Chromium
```

### 4. Native Qt

```bash
cd external_falsification/qt
mkdir build && cd build
cmake ..
make
./qt_benchmark
```

### 5. ImGui (Optional)

```bash
cd external_falsification/imgui
mkdir build && cd build
cmake ..
make
./imgui_benchmark
```

## Analyzing Results

After running all benchmarks, use the comparison tool:

```bash
cd external_falsification
python3 analyze_results.py
```

This will generate:
- Comparative metrics table
- Variance visualization
- Honest assessment of wins/losses
- Recommendations for improvement

## Expected Results

Based on Aegis's architectural design:

### Aegis Should Win On:
- ✅ **Frame Variance**: Order of magnitude lower variance
- ✅ **P99 Predictability**: Tight tail latency distribution
- ✅ **Memory Stability**: Deterministic allocation patterns
- ✅ **Replay Fidelity**: Byte-for-byte reproducibility

### Alternative Stacks May Win On:
- ⚠️ **Average Frame Time** (in some cases): JIT optimization can be fast
- ⚠️ **Development Velocity**: Higher-level frameworks are faster to develop with
- ⚠️ **Ecosystem**: More libraries and tools available

### Honest Assessment

We document ALL results, including:
- Where Aegis wins (variance, predictability)
- Where alternatives win (if any)
- Why each result occurs (technical explanation)
- What we can learn and improve

## Implementation Status

- [x] Aegis baseline (existing)
- [x] Chromium + React
- [x] Chromium + Canvas  
- [x] Native Qt
- [ ] ImGui (optional - not critical)
- [x] Results analysis tool
- [x] Comparative documentation

## Quick Start Guide

### Automated Testing (GitHub Actions)

The external falsification benchmarks include a GitHub Actions workflow that automatically:
- Builds and runs Aegis baseline benchmark
- Validates all implementations build correctly
- Tests the analysis tool
- Generates comprehensive results

See [WORKFLOW.md](WORKFLOW.md) for details.

### Manual Testing

### 1. Run All Benchmarks

```bash
# From the external_falsification directory

# Aegis (baseline)
cd ..
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/core/browser_comparison_demo > external_falsification/aegis_results.txt

# React
cd external_falsification/react
npm install
npm run dev
# Open browser, run benchmark, export results as react_results.json
# Move react_results.json to external_falsification/

# Canvas
cd ../canvas
# Open canvas_benchmark.html in Chrome
# Run benchmark, export results as canvas_results.json
# Move canvas_results.json to external_falsification/

# Qt
cd ../qt
mkdir -p build && cd build
cmake ..
make
./qt_benchmark
# Run benchmark in GUI, export results as qt_results.json
# Move qt_results.json to external_falsification/
```

### 2. Analyze Results

```bash
cd external_falsification
python3 analyze_results.py
```

This will generate a comprehensive comparison showing:
- Where Aegis wins (variance, P99)
- Where alternatives win (if any)
- Honest explanations for all results
- Recommendations for improvement

## Result Files

Place exported JSON files in the `external_falsification/` directory:

- `aegis_results.json` - Aegis baseline (convert from C++ output)
- `react_results.json` - React implementation
- `canvas_results.json` - Canvas implementation
- `qt_results.json` - Qt implementation
- `imgui_results.json` - ImGui implementation (optional)

## References

- [Aegis Browser Comparison](../core/benchmark/BROWSER_COMPARISON.md)
- [Determinism Contract](../docs/DETERMINISM.md)
- [Aegis Manifesto](../MANIFESTO.md)
