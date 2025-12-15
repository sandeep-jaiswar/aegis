# Aegis Deterministic Benchmark Harness

## Overview

The Aegis Benchmark Harness provides deterministic, replayable performance benchmarking with comprehensive statistical metrics. It is designed to measure correctness and performance in accordance with the Aegis architectural principles:

- **Deterministic execution**: Same inputs → identical outputs
- **Comprehensive metrics**: P50, P90, P95, P99, P99.9 percentiles
- **Regression detection**: Automated performance regression checks
- **Workload replay**: Record and replay workloads for consistent testing
- **Zero dependencies**: Core-only implementation with no OS dependencies

## Key Features

### 1. Percentile Metrics

The harness collects detailed timing statistics:

- **P50 (Median)**: Typical performance
- **P90**: Performance experienced by 90% of operations
- **P95**: Performance experienced by 95% of operations
- **P99**: Tail latency metric (critical for deterministic systems)
- **P99.9**: Extreme tail latency
- **Min/Max/Mean**: Additional statistical measures

### 2. Deterministic Workload Recording

Record operations for reproducible benchmarks:

```cpp
// Create recorder
benchmark::workload_recorder recorder(event_buffer, max_events);

// Record events
recorder.start_recording("my_workload");
recorder.record_event(workload_event_type::allocate, timestamp, size, alignment);
recorder.record_event(workload_event_type::operation, timestamp, op_id);

// Finish and get workload
benchmark::workload work = recorder.finish_recording();
```

### 3. Workload Replay

Replay recorded workloads with hash verification:

```cpp
// Create player
benchmark::workload_player player(work);

// Replay events
while (player.has_more_events()) {
    const workload_event* evt = player.next_event();
    // Execute event
}

// Verify hash matches original recording
uint64_t replay_hash = player.get_hash();
```

### 4. Regression Detection

Automatically detect performance regressions in CI:

```cpp
// Compare baseline vs current results
auto comparison = compare_results(baseline, current, 0.05); // 5% threshold

if (comparison == comparison_result::regressed) {
    // Performance regression detected!
}
```

## Architecture

### Components

```
benchmark/
├── benchmark.hpp              # Benchmark interface and base types
├── benchmark_result.hpp       # Result types and comparison
├── benchmark_runner.hpp       # Benchmark execution engine
├── workload.hpp              # Workload recording and replay
├── frame_allocator_benchmark.hpp   # Example: Frame allocator benchmark
├── frame_lifecycle_benchmark.hpp   # Example: Frame lifecycle benchmark
└── demo_benchmark.cpp        # Demonstration and usage examples
```

### Design Principles

1. **No OS Dependencies**: Uses deterministic timestamp providers for portability
2. **Explicit Memory Management**: All buffers provided by caller
3. **Zero Allocations**: No hidden allocations in hot paths
4. **Compile-time Configuration**: No runtime configuration overhead

## Usage

### Creating a Benchmark

Implement the `benchmark` interface:

```cpp
class my_benchmark final : public benchmark {
public:
    const char* name() const noexcept override {
        return "my_benchmark";
    }

    void setup() noexcept override {
        // Initialize resources
    }

    void execute() noexcept override {
        // Benchmark hot path (measured)
    }

    void teardown() noexcept override {
        // Clean up resources
    }

    bool verify() const noexcept override {
        // Verify correctness
        return true;
    }

    uint64_t get_workload_hash() const noexcept override {
        // Return workload identifier
        return 0x123456789ABCDEF0ULL;
    }
};
```

### Running a Benchmark

```cpp
// Create benchmark
my_benchmark bench;

// Allocate timing buffer
constexpr size_t max_iterations = 1000;
uint64_t* timing_buffer = new uint64_t[max_iterations];

// Create runner
benchmark::deterministic_timestamp_provider timer;
benchmark::benchmark_runner runner(timing_buffer, max_iterations, &timer);

// Configure
benchmark::benchmark_config config{};
config.warmup_iterations = 10;
config.measured_iterations = 100;

// Run and get results
benchmark::benchmark_result result = runner.run(bench, config);

// Use results
printf("P99: %llu ns\n", result.timing.p99_ns);
```

## CI Integration

The benchmark harness integrates with GitHub Actions for regression detection:

1. **Baseline Recording**: Establish baseline performance on main branch
2. **Pull Request Checks**: Compare PR performance against baseline
3. **Regression Detection**: Fail CI if performance regresses beyond threshold
4. **Metrics Reporting**: Report P50/P99/P99.9 in CI output

See `.github/workflows/build.yml` for CI configuration.

## Benchmark Examples

### Frame Allocator Benchmark

Measures frame allocator performance with typical allocation patterns:

```cpp
benchmark::frame_allocator_benchmark bench(&allocator);
benchmark::benchmark_result result = runner.run(bench, config);
```

### Frame Lifecycle Benchmark

Measures complete frame execution cycle:

```cpp
benchmark::frame_lifecycle_benchmark bench(&context, &executor);
benchmark::benchmark_result result = runner.run(bench, config);
```

## Performance Characteristics

The benchmark harness itself is designed to minimize overhead:

- **Timing Collection**: O(1) per iteration
- **Percentile Calculation**: O(n log n) sorting, done once after measurement
- **Memory Usage**: Fixed, based on iteration count
- **No Allocations**: All memory provided by caller

## Best Practices

1. **Warmup Iterations**: Always include warmup to account for cache effects
2. **Sufficient Iterations**: Use 100+ iterations for stable percentile metrics
3. **Workload Hashing**: Use meaningful hashes to detect workload changes
4. **Verification**: Implement `verify()` to catch correctness regressions
5. **Deterministic Timing**: Use deterministic timestamp providers for reproducibility

## Implementation Notes

### Timestamp Providers

The harness uses a `timestamp_provider` interface for timing:

- **Deterministic Provider**: Simple counter for testing (default)
- **Platform Provider**: Real wall-clock time for actual benchmarks
- **Injected Provider**: Custom timing for advanced use cases

### Percentile Calculation

Percentiles are calculated from sorted timing data:

1. Collect all iteration timings
2. Sort timing array
3. Extract percentile values at specific indices
4. Calculate mean from sum

### Hash Verification

Workload hashes use FNV-1a algorithm:

- Fast computation
- Good distribution
- Suitable for detecting workload changes
- Not cryptographic (doesn't need to be)

## Future Enhancements

Potential future additions (not in initial implementation):

- Workload serialization to disk
- Benchmark result export (JSON/CSV)
- Comparative visualization
- Multi-threaded benchmark support
- Memory allocation profiling integration
- Custom metric collectors

## References

- [Aegis Architecture](../../docs/ARCHITECTURE.md)
- [Core Folder Contract](../../docs/CORE_FOLDER_CONTRACT.md)
- [Frame Lifecycle](../frame/README.md)
- [Memory System](../../docs/MEMORY_SYSTEM.md)
