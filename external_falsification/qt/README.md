# APP-001 Qt Benchmark

## Overview

This is the Qt native GUI implementation of the APP-001 benchmark workload for external falsification of Aegis's performance claims.

## Technology Stack

- **Framework**: Qt 6
- **Language**: C++17
- **Build System**: CMake
- **Rendering**: Native platform widgets

## Prerequisites

- CMake 3.16+
- Qt 6 (QtCore and QtWidgets)
- C++17 compatible compiler

### Installing Qt 6

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install qt6-base-dev qt6-tools-dev cmake
```

#### macOS (with Homebrew)
```bash
brew install qt@6 cmake
```

#### Windows
Download and install Qt 6 from https://www.qt.io/download

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running

```bash
./qt_benchmark
```

## Using the Benchmark

1. Launch the qt_benchmark application
2. Click "Start Benchmark"
3. Wait for completion (~10-15 seconds)
4. Review the metrics displayed in the text area
5. Click "Export Results" to save as JSON

## What This Measures

This implementation exercises the Qt framework:

- **Qt Event Loop**: Signal/slot mechanism and event processing
- **QObject System**: Memory management and object hierarchy
- **Native Widgets**: Platform-specific rendering
- **Qt Containers**: std::vector for allocations
- **C++ Memory**: malloc/free for workload allocations

## Expected Results

Compared to Aegis:

- ⚠️ **Mixed variance**: Better than browser JavaScript, worse than Aegis
- ⚠️ **Better P99 than browser**: Native code, no GC pauses
- ✅ **Good average performance**: C++ is fast
- ❌ **Some variance**: Qt event loop and platform windowing system
- ❌ **Poor replay fidelity**: Platform-dependent timing

## Files

- `CMakeLists.txt` - CMake build configuration
- `src/main.cpp` - Application entry point
- `src/benchmark_window.h/cpp` - Main benchmark window
- `src/workload.h/cpp` - Workload implementation
- `README.md` - This file

## Exporting Results

Results are exported in JSON format:

```json
{
  "technology": "Qt",
  "timestamp": "2024-01-01T00:00:00",
  "config": {
    "warmupIterations": 10,
    "measuredIterations": 100,
    "allocationsPerFrame": 50,
    "operationsPerFrame": 100
  },
  "results": {
    "min_ns": 15000,
    "p50_ns": 25000,
    "p90_ns": 45000,
    "p95_ns": 55000,
    "p99_ns": 85000,
    "p99_9_ns": 120000,
    "max_ns": 180000,
    "mean_ns": 32000,
    "variance": 1250000000
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

## Implementation Notes

- Uses std::chrono::high_resolution_clock for timing
- Allocations use standard malloc/free
- Qt event loop runs in background
- Results displayed in native Qt widgets
- Cross-platform compatible (Linux, macOS, Windows)
