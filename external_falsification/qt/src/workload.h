#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace aegis::benchmark {

// Benchmark configuration (matching APP-001 spec)
struct Config {
    static constexpr size_t warmup_iterations = 10;
    static constexpr size_t measured_iterations = 100;
    static constexpr size_t allocations_per_frame = 50;
    static constexpr size_t operations_per_frame = 100;
    static constexpr size_t allocation_sizes[] = {64, 128, 256, 512, 1024};
    static constexpr size_t allocation_size_count = 5;
};

// Workload implementation
class QtWorkload {
  public:
    QtWorkload();
    ~QtWorkload();

    void reset();
    void execute_frame();

  private:
    void perform_allocations();
    void perform_operations();

    std::vector<void*> allocations_;
    std::vector<uint64_t> operations_;
};

// Percentile metrics
struct PercentileMetrics {
    uint64_t min_ns;
    uint64_t p50_ns;
    uint64_t p90_ns;
    uint64_t p95_ns;
    uint64_t p99_ns;
    uint64_t p99_9_ns;
    uint64_t max_ns;
    uint64_t mean_ns;
    double variance;
};

// Calculate percentiles from timing data
PercentileMetrics calculate_percentiles(const std::vector<uint64_t>& timings);

} // namespace aegis::benchmark
