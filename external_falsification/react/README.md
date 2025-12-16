# APP-001 React Benchmark

## Overview

This is the React implementation of the APP-001 benchmark workload for external falsification of Aegis's performance claims.

## Technology Stack

- **Framework**: React 18.2
- **Build Tool**: Vite 5.0
- **Runtime**: Chromium/Chrome browser

## Running the Benchmark

### Prerequisites

- Node.js 18+ and npm

### Installation

```bash
npm install
```

### Development Mode

```bash
npm run dev
```

Then open http://localhost:3000 in your browser.

### Production Build

```bash
npm run build
npm run preview
```

## Using the Benchmark

1. Open the application in Chrome/Chromium
2. Close all other tabs and applications for accurate results
3. Click "Start Benchmark"
4. Wait for completion (~10-15 seconds)
5. Review the metrics displayed
6. Click "Export Results" to save as JSON

## What This Measures

This implementation adds React-specific overhead:

- **Virtual DOM**: Reconciliation and diffing
- **Component Lifecycle**: Mounting, updating, unmounting
- **React Fiber**: Internal scheduling algorithm
- **Garbage Collection**: From React's object allocation
- **JavaScript Engine**: V8 JIT compilation variance

## Expected Results

Compared to Aegis:

- ❌ **Higher variance**: React's virtual DOM and GC cause unpredictable pauses
- ❌ **Worse P99**: Tail latency affected by framework overhead
- ⚠️ **Comparable average**: JIT optimization can be fast on average
- ❌ **Poor replay fidelity**: Non-deterministic GC timing

## Files

- `src/App.jsx` - Main benchmark implementation
- `src/App.css` - Styling
- `src/main.jsx` - React entry point
- `src/index.css` - Global styles
- `package.json` - Dependencies
- `vite.config.js` - Vite configuration

## Exporting Results

Results are exported in JSON format:

```json
{
  "technology": "React",
  "timestamp": "2024-01-01T00:00:00.000Z",
  "config": {
    "warmupIterations": 10,
    "measuredIterations": 100,
    "allocationsPerFrame": 50,
    "operationsPerFrame": 100
  },
  "results": {
    "min_ns": 50000,
    "p50_ns": 75000,
    "p90_ns": 150000,
    "p95_ns": 200000,
    "p99_ns": 350000,
    "p99_9_ns": 500000,
    "max_ns": 750000,
    "mean_ns": 90000,
    "variance": 12500000000
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
