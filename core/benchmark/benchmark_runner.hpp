#pragma once

#include "core/benchmark/benchmark.hpp"
#include "core/benchmark/benchmark_result.hpp"
#include "core/memory/allocator.hpp"

#include <cstddef>
#include <cstdint>

namespace aegis::core::benchmark {

// Timestamp provider interface - allows deterministic time injection
class timestamp_provider {
  public:
    virtual ~timestamp_provider() noexcept = default;

    // Get current timestamp in nanoseconds
    [[nodiscard]] virtual uint64_t now_ns() const noexcept = 0;
};

// Default timestamp provider using platform time
// In real implementation, this would use a platform abstraction
// For now, we provide a simple monotonic counter for determinism
class deterministic_timestamp_provider final : public timestamp_provider {
  public:
    deterministic_timestamp_provider() noexcept = default;

    [[nodiscard]] uint64_t now_ns() const noexcept override {
        // In a real implementation, this would use platform timing
        // For deterministic testing, we use a simple counter
        return counter++;
    }

  private:
    mutable uint64_t counter{0};
};

// Benchmark runner - executes benchmarks and collects results
class benchmark_runner {
  public:
    // Create benchmark runner with timing array buffer
    // timing_buffer must have space for at least max_iterations entries
    explicit benchmark_runner(uint64_t* timing_buffer_ptr, size_t max_iterations_val,
                              timestamp_provider* timer_ptr = nullptr) noexcept
        : timing_buffer(timing_buffer_ptr), max_iterations(max_iterations_val), timer(timer_ptr) {
        if (timer == nullptr) {
            timer = &default_timer;
        }
    }

    ~benchmark_runner() noexcept = default;

    // Disable copy and move
    benchmark_runner(const benchmark_runner&) = delete;
    benchmark_runner& operator=(const benchmark_runner&) = delete;
    benchmark_runner(benchmark_runner&&) = delete;
    benchmark_runner& operator=(benchmark_runner&&) = delete;

    // Run benchmark with specified configuration
    // Returns benchmark result with timing and memory statistics
    [[nodiscard]] benchmark_result run(benchmark& bench, const benchmark_config& config) noexcept {
        benchmark_result result{};
        result.name = bench.name();
        result.workload_hash = bench.get_workload_hash();

        // Validate configuration
        const uint64_t total_iterations = config.warmup_iterations + config.measured_iterations;
        if (total_iterations > max_iterations) {
            return result; // Return empty result on error
        }

        // Setup benchmark
        bench.setup();

        // Warmup iterations
        for (uint64_t i = 0; i < config.warmup_iterations; ++i) {
            bench.execute();
        }

        // Measured iterations
        for (uint64_t i = 0; i < config.measured_iterations; ++i) {
            const uint64_t start_ns = timer->now_ns();
            bench.execute();
            const uint64_t end_ns = timer->now_ns();
            timing_buffer[i] = end_ns - start_ns;
        }

        // Teardown benchmark
        bench.teardown();

        // Verify correctness
        result.deterministic = bench.verify();

        // Calculate statistics
        result.iterations = config.measured_iterations;
        result.total_time_ns = 0;
        for (uint64_t i = 0; i < config.measured_iterations; ++i) {
            result.total_time_ns += timing_buffer[i];
        }

        // Sort timing data for percentile calculation
        sort_uint64_array(timing_buffer, config.measured_iterations);

        // Calculate percentiles
        calculate_percentiles(timing_buffer, config.measured_iterations, result.timing);

        return result;
    }

    // Set custom timestamp provider
    void set_timestamp_provider(timestamp_provider* provider) noexcept {
        timer = provider ? provider : &default_timer;
    }

  private:
    uint64_t* timing_buffer;
    size_t max_iterations;
    timestamp_provider* timer;
    deterministic_timestamp_provider default_timer{};
};

} // namespace aegis::core::benchmark
