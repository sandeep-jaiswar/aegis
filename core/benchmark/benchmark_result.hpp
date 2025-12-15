#pragma once

#include <cstddef>
#include <cstdint>

namespace aegis::core::benchmark {

// Statistical metrics for benchmark results
struct percentile_metrics {
    uint64_t p50_ns{0};   // Median (50th percentile)
    uint64_t p90_ns{0};   // 90th percentile
    uint64_t p95_ns{0};   // 95th percentile
    uint64_t p99_ns{0};   // 99th percentile
    uint64_t p99_9_ns{0}; // 99.9th percentile
    uint64_t min_ns{0};   // Minimum time
    uint64_t max_ns{0};   // Maximum time
    uint64_t mean_ns{0};  // Average time
};

// Benchmark result containing timing and memory statistics
struct benchmark_result {
    const char* name{nullptr};       // Benchmark name
    uint64_t iterations{0};          // Number of iterations executed
    uint64_t total_time_ns{0};       // Total time for all iterations
    percentile_metrics timing{};     // Timing percentile metrics
    size_t total_bytes_allocated{0}; // Total bytes allocated
    size_t peak_bytes_used{0};       // Peak memory usage
    size_t total_allocations{0};     // Total allocation count
    bool deterministic{true};        // Whether results are deterministic
    uint64_t workload_hash{0};       // Hash of workload for replay verification
};

// Result comparison for regression detection
enum class comparison_result : uint8_t {
    improved = 0,    // New result is better
    unchanged = 1,   // Results are equivalent
    regressed = 2,   // New result is worse
    incomparable = 3 // Cannot compare (different workloads)
};

// Compare two benchmark results
// Returns whether there is a regression based on P99 threshold
[[nodiscard]] inline comparison_result
compare_results(const benchmark_result& baseline, const benchmark_result& current,
                double regression_threshold = 0.05) noexcept {
    // Check if workloads are comparable
    if (baseline.workload_hash != current.workload_hash) {
        return comparison_result::incomparable;
    }

    // Compare P99 latency (most important metric for deterministic systems)
    const auto baseline_p99 = baseline.timing.p99_ns;
    const auto current_p99 = current.timing.p99_ns;

    if (baseline_p99 == 0) {
        return comparison_result::unchanged;
    }

    const double change_ratio =
        static_cast<double>(current_p99 - baseline_p99) / static_cast<double>(baseline_p99);

    if (change_ratio > regression_threshold) {
        return comparison_result::regressed;
    } else if (change_ratio < -regression_threshold) {
        return comparison_result::improved;
    } else {
        return comparison_result::unchanged;
    }
}

} // namespace aegis::core::benchmark
