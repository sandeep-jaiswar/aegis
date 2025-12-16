# Aegis Benchmark Corpus Specification

## Overview

This document specifies the canonical benchmark workload corpus for Aegis runtime performance evaluation. The corpus provides standardized, reproducible workloads that can be used by the Aegis team and third parties to measure and compare runtime performance.

**Version**: 1.0.0  
**Status**: Frozen  
**Ticket**: BEN-A001

## Goals

1. **Deterministic Replay**: All workloads must replay identically across runs
2. **Byte-Stable Outputs**: Benchmark outputs must be byte-for-byte identical
3. **Third-Party Usability**: External parties can run benchmarks without modification
4. **Performance Envelopes**: Define expected performance bounds for validation

## Corpus Structure

```
benchmarks/
├── corpus/
│   ├── CORPUS_SPECIFICATION.md        # This file
│   ├── data_grid_workload.json        # Data grid operations workload
│   ├── event_stream_workload.json     # Event processing workload
│   └── performance_envelopes.json     # Expected performance bounds
└── results/
    └── baseline/                       # Reference baseline results
        ├── data_grid_baseline.json
        └── event_stream_baseline.json
```

## Workload Definitions

### 1. Data Grid Workload

**Purpose**: Simulates typical data grid operations with allocations, lookups, and updates.

**Operations**:
- Frame allocations: 50 allocations per frame
- Allocation sizes: [64, 128, 256, 512, 1024] bytes (cycling pattern)
- Alignment: 8-byte alignment for all allocations
- Operations per frame: 100 computational operations
- Frame count: 1000 frames

**Determinism**:
- Fixed allocation pattern
- Deterministic operation sequence
- Reproducible memory layout
- Hash verification: Workload hash must match `0xDADA6D1DC0DE0001`

**Expected Performance Envelope** (Release build, x86_64):
- P50: 1,000 - 2,000 ns
- P99: 2,000 - 4,000 ns
- P99.9: 3,000 - 6,000 ns
- Max variance: < 10,000,000 ns²

### 2. Event Stream Workload

**Purpose**: Simulates event processing pipeline with variable-sized events.

**Operations**:
- Event types: 5 different event types
- Events per frame: 20 events
- Event payload sizes: [32, 64, 96, 128, 160] bytes
- Processing operations: 50 operations per event
- Frame count: 1000 frames

**Determinism**:
- Fixed event sequence
- Deterministic event ordering
- Reproducible processing order
- Hash verification: Workload hash must match `0xEEEE57123EA40002`

**Expected Performance Envelope** (Release build, x86_64):
- P50: 1,500 - 3,000 ns
- P99: 3,000 - 6,000 ns
- P99.9: 5,000 - 10,000 ns
- Max variance: < 15,000,000 ns²

## Acceptance Criteria

### ✅ Workloads Replay Identically

**Verification Method**:
1. Run workload twice
2. Compare workload hashes
3. Hashes must match exactly

**Test Command**:
```bash
./build/benchmarks/verify_corpus_replay
```

**Expected Output**:
```
Data Grid Workload - Hash Match: ✓
Event Stream Workload - Hash Match: ✓
All workloads replay identically: PASS
```

### ✅ Outputs Byte-Stable

**Verification Method**:
1. Run benchmark and save results to JSON
2. Run again and compare JSON byte-for-byte
3. Files must be identical (excluding timestamps)

**Test Command**:
```bash
./build/benchmarks/run_corpus --output results/run1.json
./build/benchmarks/run_corpus --output results/run2.json
diff -u <(jq 'del(.timestamp)' results/run1.json) \
        <(jq 'del(.timestamp)' results/run2.json)
```

**Expected Output**:
```
(no differences - files are identical)
```

### ✅ Benchmarks Usable by Third Parties

**Requirements**:
1. Complete documentation of workload format
2. Reference implementation provided
3. Validation tools included
4. No proprietary dependencies
5. Results exportable in standard JSON format

**Third-Party Usage**:
```bash
# Clone repository
git clone https://github.com/sandeep-jaiswar/aegis.git
cd aegis

# Build with standard toolchain
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run corpus benchmarks
./build/benchmarks/run_corpus --output my_results.json

# Validate against reference
./build/benchmarks/validate_results my_results.json \
    benchmarks/results/baseline/
```

## Workload File Format

Workload files use JSON format with the following structure:

```json
{
  "workload_name": "data_grid_workload",
  "version": "1.0.0",
  "workload_hash": "0xDADA6D1DC0DE0001",
  "description": "Data grid operations benchmark",
  "frame_count": 1000,
  "operations_per_frame": 100,
  "events": [
    {
      "type": "allocate",
      "timestamp_ns": 0,
      "size": 64,
      "alignment": 8,
      "param3": 0
    },
    {
      "type": "operation",
      "timestamp_ns": 100,
      "op_id": 1,
      "param2": 0,
      "param3": 0
    }
  ]
}
```

## Performance Envelope Format

Performance envelopes define expected performance bounds:

```json
{
  "workload": "data_grid_workload",
  "platform": "x86_64",
  "build_type": "Release",
  "compiler": "gcc-13.2",
  "envelope": {
    "p50_ns": {
      "min": 1000,
      "max": 2000
    },
    "p99_ns": {
      "min": 2000,
      "max": 4000
    },
    "p99_9_ns": {
      "min": 3000,
      "max": 6000
    },
    "variance": {
      "max": 10000000
    }
  }
}
```

## Determinism Guarantees

All corpus workloads guarantee:

1. **Input Determinism**: Same workload file produces same execution sequence
2. **Execution Determinism**: Same execution sequence produces same memory operations
3. **Output Determinism**: Same memory operations produce same results
4. **Hash Stability**: Workload hash is stable across runs and platforms

## Validation Process

### Pre-Freeze Validation

Before freezing a workload version:

1. Run workload 100 times
2. Verify all hashes match
3. Verify all results are byte-stable
4. Verify performance within envelope
5. Test on multiple platforms (x86_64, ARM64)
6. Test with multiple compilers (GCC, Clang)

### Post-Freeze Validation

Once frozen, workloads must not change. Any modification requires:

1. Version bump (e.g., 1.0.0 → 1.1.0)
2. New workload hash
3. New performance envelopes
4. Migration guide from previous version

## Usage Examples

### Running a Single Workload

```cpp
#include "benchmarks/corpus/corpus_runner.hpp"

// Load workload from JSON
auto workload = aegis::benchmark::load_workload_from_json(
    "benchmarks/corpus/data_grid_workload.json"
);

// Run benchmark
auto result = aegis::benchmark::run_corpus_workload(workload);

// Verify hash
assert(result.workload_hash == 0xDADA6D1DC0DE0001ULL);

// Check performance envelope
auto envelope = aegis::benchmark::load_envelope(
    "benchmarks/corpus/performance_envelopes.json", 
    "data_grid_workload"
);

bool within_envelope = aegis::benchmark::validate_performance(
    result, envelope
);
```

### Exporting Results

```cpp
// Export to JSON
aegis::benchmark::export_result_json(
    result,
    "benchmarks/results/my_run.json"
);
```

## Future Extensions

Potential future workloads (not in v1.0.0):

- [ ] Layout computation workload
- [ ] Diff algorithm workload  
- [ ] Multi-frame sequence workload
- [ ] Memory stress workload
- [ ] Real-world UI simulation workload

## References

- [Aegis Determinism Contract](../../docs/DETERMINISM.md)
- [Benchmark Harness](../../core/benchmark/README.md)
- [Browser Comparison](../../core/benchmark/BROWSER_COMPARISON.md)
- BEN-A001: Canonical Workload Set (this ticket)
- BEN-A002: Browser & Native Comparison Report

## Changelog

### Version 1.0.0 (Initial Release)
- Initial corpus specification
- Data grid workload defined
- Event stream workload defined
- Performance envelopes established
- Validation criteria documented
