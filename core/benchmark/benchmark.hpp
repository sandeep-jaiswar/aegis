#pragma once

#include "core/benchmark/benchmark_result.hpp"
#include "core/memory/allocator.hpp"

#include <cstddef>
#include <cstdint>

namespace aegis::core::benchmark {

// Benchmark configuration
struct benchmark_config {
    uint64_t warmup_iterations{10};   // Iterations before measuring
    uint64_t measured_iterations{100}; // Iterations to measure
    bool record_workload{false};       // Record workload for replay
    const char* workload_file{nullptr}; // File to save/load workload
};

// Benchmark interface - all benchmarks implement this
class benchmark {
  public:
    benchmark() noexcept = default;
    virtual ~benchmark() noexcept = default;

    // Disable copy and move
    benchmark(const benchmark&) = delete;
    benchmark& operator=(const benchmark&) = delete;
    benchmark(benchmark&&) = delete;
    benchmark& operator=(benchmark&&) = delete;

    // Get benchmark name
    [[nodiscard]] virtual const char* name() const noexcept = 0;

    // Setup before benchmark execution
    // Called once before warmup iterations
    virtual void setup() noexcept {}

    // Teardown after benchmark execution
    // Called once after all iterations complete
    virtual void teardown() noexcept {}

    // Execute one iteration of the benchmark
    // This is the hot path that gets measured
    virtual void execute() noexcept = 0;

    // Verify correctness of benchmark execution
    // Returns true if results are correct, false otherwise
    [[nodiscard]] virtual bool verify() const noexcept {
        return true;
    }

    // Get workload hash for deterministic replay verification
    [[nodiscard]] virtual uint64_t get_workload_hash() const noexcept {
        return 0;
    }
};

// Helper to calculate percentiles from sorted timing data
// data must be sorted in ascending order
// data_size is the number of samples
inline void calculate_percentiles(const uint64_t* data,
                                  size_t data_size,
                                  percentile_metrics& out_metrics) noexcept {
    if (data_size == 0) {
        return;
    }

    // Calculate percentile indices
    const auto p50_idx = (data_size * 50) / 100;
    const auto p90_idx = (data_size * 90) / 100;
    const auto p95_idx = (data_size * 95) / 100;
    const auto p99_idx = (data_size * 99) / 100;
    const auto p99_9_idx = (data_size * 999) / 1000;

    out_metrics.p50_ns = data[p50_idx < data_size ? p50_idx : data_size - 1];
    out_metrics.p90_ns = data[p90_idx < data_size ? p90_idx : data_size - 1];
    out_metrics.p95_ns = data[p95_idx < data_size ? p95_idx : data_size - 1];
    out_metrics.p99_ns = data[p99_idx < data_size ? p99_idx : data_size - 1];
    out_metrics.p99_9_ns = data[p99_9_idx < data_size ? p99_9_idx : data_size - 1];
    out_metrics.min_ns = data[0];
    out_metrics.max_ns = data[data_size - 1];

    // Calculate mean
    uint64_t sum = 0;
    for (size_t i = 0; i < data_size; ++i) {
        sum += data[i];
    }
    out_metrics.mean_ns = sum / data_size;
}

// Simple insertion sort for small arrays (good for cache locality)
inline void sort_uint64_array(uint64_t* data, size_t size) noexcept {
    for (size_t i = 1; i < size; ++i) {
        const uint64_t key = data[i];
        size_t j = i;
        while (j > 0 && data[j - 1] > key) {
            data[j] = data[j - 1];
            --j;
        }
        data[j] = key;
    }
}

} // namespace aegis::core::benchmark
