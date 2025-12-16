#include "core/benchmark/benchmark.hpp"
#include "core/benchmark/benchmark_result.hpp"
#include "core/benchmark/benchmark_runner.hpp"
#include "core/benchmark/browser_comparison_benchmark.hpp"
#include "core/memory/frame_allocator.hpp"

#include <chrono>
#include <cstdio>

using namespace aegis::core;

// Platform timestamp provider using std::chrono for real timing
class platform_timestamp_provider final : public benchmark::timestamp_provider {
  public:
    platform_timestamp_provider() noexcept = default;

    [[nodiscard]] uint64_t now_ns() const noexcept override {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
    }
};

// Helper to print percentile metrics
void print_percentiles(const benchmark::percentile_metrics& metrics) {
    printf("  Min:    %10llu ns\n", static_cast<unsigned long long>(metrics.min_ns));
    printf("  P50:    %10llu ns\n", static_cast<unsigned long long>(metrics.p50_ns));
    printf("  P90:    %10llu ns\n", static_cast<unsigned long long>(metrics.p90_ns));
    printf("  P95:    %10llu ns\n", static_cast<unsigned long long>(metrics.p95_ns));
    printf("  P99:    %10llu ns\n", static_cast<unsigned long long>(metrics.p99_ns));
    printf("  P99.9:  %10llu ns\n", static_cast<unsigned long long>(metrics.p99_9_ns));
    printf("  Max:    %10llu ns\n", static_cast<unsigned long long>(metrics.max_ns));
    printf("  Mean:   %10llu ns\n", static_cast<unsigned long long>(metrics.mean_ns));

    // Calculate actual variance from timing data
    // This is a rough approximation for display
    const uint64_t range = metrics.max_ns - metrics.min_ns;
    const uint64_t variance_estimate = (range * range) / 12;

    printf("  Variance (est): %10llu ns²\n", static_cast<unsigned long long>(variance_estimate));
}

// Helper to print benchmark results
void print_result(const benchmark::benchmark_result& result) {
    printf("\n=== Benchmark: %s ===\n", result.name);
    printf("Iterations:    %llu\n", static_cast<unsigned long long>(result.iterations));
    printf("Total Time:    %llu ns\n", static_cast<unsigned long long>(result.total_time_ns));
    printf("Workload Hash: 0x%016llx\n", static_cast<unsigned long long>(result.workload_hash));
    printf("\nTiming Metrics:\n");
    print_percentiles(result.timing);
    printf("\n");
}

// Print JSON output for easy comparison with browser results
void print_json_results(const benchmark::benchmark_result& result) {
    printf("\n=== JSON Output (for comparison with browser) ===\n");
    printf("{\n");
    printf("  \"benchmark\": \"%s\",\n", result.name);
    printf("  \"timestamp\": \"<current_time>\",\n");
    printf("  \"aegis\": {\n");
    printf("    \"min_ns\": %llu,\n", static_cast<unsigned long long>(result.timing.min_ns));
    printf("    \"p50_ns\": %llu,\n", static_cast<unsigned long long>(result.timing.p50_ns));
    printf("    \"p90_ns\": %llu,\n", static_cast<unsigned long long>(result.timing.p90_ns));
    printf("    \"p95_ns\": %llu,\n", static_cast<unsigned long long>(result.timing.p95_ns));
    printf("    \"p99_ns\": %llu,\n", static_cast<unsigned long long>(result.timing.p99_ns));
    printf("    \"p99_9_ns\": %llu,\n", static_cast<unsigned long long>(result.timing.p99_9_ns));
    printf("    \"max_ns\": %llu,\n", static_cast<unsigned long long>(result.timing.max_ns));
    printf("    \"mean_ns\": %llu,\n", static_cast<unsigned long long>(result.timing.mean_ns));

    // Calculate variance estimate
    const uint64_t range = result.timing.max_ns - result.timing.min_ns;
    const uint64_t variance_estimate = (range * range) / 12;
    printf("    \"variance\": %llu\n", static_cast<unsigned long long>(variance_estimate));

    printf("  }\n");
    printf("}\n");
}

int main() {
    printf("╔════════════════════════════════════════════╗\n");
    printf("║  Aegis Real-Time Benchmark                 ║\n");
    printf("║  APP-001 with Platform Timing              ║\n");
    printf("╚════════════════════════════════════════════╝\n");

    // Allocate buffer for frame allocator
    constexpr size_t buffer_size = 1024 * 1024; // 1 MB
    auto* buffer = new uint8_t[buffer_size];
    memory::frame_allocator allocator(buffer, buffer_size);

    // Create benchmark
    benchmark::browser_comparison_benchmark bench(&allocator);

    // Allocate timing buffer
    constexpr size_t max_iterations = 1000;
    auto* timing_buffer = new uint64_t[max_iterations];

    // Create runner with REAL platform timer (not deterministic)
    platform_timestamp_provider timer;
    benchmark::benchmark_runner runner(timing_buffer, max_iterations, &timer);

    // Configure benchmark to match browser settings
    benchmark::benchmark_config config{};
    config.warmup_iterations = 10;
    config.measured_iterations = 100;

    printf("\n=== Running Aegis Benchmark with Real Timing ===\n");
    printf("Configuration:\n");
    printf("  Warmup iterations:    %llu\n",
           static_cast<unsigned long long>(config.warmup_iterations));
    printf("  Measured iterations:  %llu\n",
           static_cast<unsigned long long>(config.measured_iterations));
    printf("  Allocations/frame:    %zu\n",
           benchmark::browser_comparison_config::allocations_per_frame);
    printf("  Operations/frame:     %zu\n",
           benchmark::browser_comparison_config::operations_per_frame);
    printf("\n");
    printf("⚠️  Note: Using real platform timing (not deterministic)\n");
    printf("   Results will show actual variance and may differ between runs.\n");
    printf("\n");

    // Run benchmark
    benchmark::benchmark_result result = runner.run(bench, config);

    // Print results
    print_result(result);

    // Print JSON for easy comparison
    print_json_results(result);

    // Print comparison information
    printf("\n=== External Falsification Comparison ===\n");
    printf("1. Save the JSON output above to external_falsification/aegis_results.json\n");
    printf("2. Run React, Canvas, and Qt benchmarks\n");
    printf("3. Export their results to external_falsification/\n");
    printf("4. Run: cd external_falsification && python3 analyze_results.py\n");
    printf("\n");

    printf("=== Expected Results ===\n");
    printf("Aegis should demonstrate:\n");
    printf("  ✓ Low variance compared to browser-based implementations\n");
    printf("  ✓ Predictable P99 latency (close to P50)\n");
    printf("  ✓ Consistent frame times across runs\n");
    printf("  ✓ No GC pauses or event loop interference\n");
    printf("\n");

    delete[] timing_buffer;
    delete[] buffer;

    return 0;
}
