#include "core/benchmark/benchmark.hpp"
#include "core/benchmark/benchmark_result.hpp"
#include "core/benchmark/benchmark_runner.hpp"
#include "core/benchmark/frame_allocator_benchmark.hpp"
#include "core/benchmark/frame_lifecycle_benchmark.hpp"
#include "core/benchmark/workload.hpp"
#include "core/frame/frame_lifecycle.hpp"
#include "core/memory/frame_allocator.hpp"

#include <cstdio>

using namespace aegis::core;

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
}

// Helper to print benchmark results
void print_result(const benchmark::benchmark_result& result) {
    printf("\n=== Benchmark: %s ===\n", result.name);
    printf("Iterations:    %llu\n", static_cast<unsigned long long>(result.iterations));
    printf("Total Time:    %llu ns\n", static_cast<unsigned long long>(result.total_time_ns));
    printf("Deterministic: %s\n", result.deterministic ? "Yes" : "No");
    printf("Workload Hash: 0x%016llx\n", static_cast<unsigned long long>(result.workload_hash));
    printf("\nTiming Metrics:\n");
    print_percentiles(result.timing);
    printf("\n");
}

// Demonstrate workload recording and replay
void demo_workload_recording() {
    printf("\n=== Workload Recording Demo ===\n");

    // Allocate event buffer
    constexpr size_t max_events = 1000;
    auto* event_buffer = new benchmark::workload_event[max_events];

    // Create recorder
    benchmark::workload_recorder recorder(event_buffer, max_events);

    // Start recording
    recorder.start_recording("sample_workload");

    // Record some events
    recorder.record_event(benchmark::workload_event_type::allocate, 1000, 256, 8);
    recorder.record_event(benchmark::workload_event_type::allocate, 2000, 512, 16);
    recorder.record_event(benchmark::workload_event_type::operation, 3000, 42);
    recorder.record_event(benchmark::workload_event_type::deallocate, 4000, 256);
    recorder.record_event(benchmark::workload_event_type::deallocate, 5000, 512);

    // Finish recording
    benchmark::workload work = recorder.finish_recording();

    printf("Recorded workload: %s\n", work.name);
    printf("Event count: %zu\n", work.event_count);
    printf("Workload hash: 0x%016llx\n", static_cast<unsigned long long>(work.hash));

    // Create player and replay
    benchmark::workload_player player(work);
    printf("\nReplaying workload:\n");

    size_t event_num = 0;
    while (player.has_more_events()) {
        const benchmark::workload_event* evt = player.next_event();
        if (evt) {
            printf("  Event %zu: type=%d, timestamp=%llu ns, param1=%llu\n", event_num++,
                   static_cast<int>(evt->type), static_cast<unsigned long long>(evt->timestamp_ns),
                   static_cast<unsigned long long>(evt->param1));
        }
    }

    // Verify hash matches
    printf("\nVerification hash: 0x%016llx (matches: %s)\n",
           static_cast<unsigned long long>(player.get_hash()),
           player.get_hash() == work.hash ? "Yes" : "No");

    delete[] event_buffer;
}

// Run frame allocator benchmark
void run_frame_allocator_benchmark() {
    printf("\n=== Running Frame Allocator Benchmark ===\n");

    // Allocate buffer for frame allocator
    constexpr size_t buffer_size = 1024 * 1024; // 1 MB
    auto* buffer = new uint8_t[buffer_size];
    memory::frame_allocator allocator(buffer, buffer_size);

    // Create benchmark
    benchmark::frame_allocator_benchmark bench(&allocator);

    // Allocate timing buffer
    constexpr size_t max_iterations = 1000;
    auto* timing_buffer = new uint64_t[max_iterations];

    // Create runner with deterministic timer
    benchmark::deterministic_timestamp_provider timer;
    benchmark::benchmark_runner runner(timing_buffer, max_iterations, &timer);

    // Configure benchmark
    benchmark::benchmark_config config{};
    config.warmup_iterations = 10;
    config.measured_iterations = 100;

    // Run benchmark
    benchmark::benchmark_result result = runner.run(bench, config);

    // Print results
    print_result(result);

    delete[] timing_buffer;
    delete[] buffer;
}

// Run frame lifecycle benchmark
void run_frame_lifecycle_benchmark() {
    printf("\n=== Running Frame Lifecycle Benchmark ===\n");

    // Create frame context and executor
    frame::frame_context context;
    frame::frame_executor executor;

    // Create benchmark
    benchmark::frame_lifecycle_benchmark bench(&context, &executor);

    // Allocate timing buffer
    constexpr size_t max_iterations = 1000;
    auto* timing_buffer = new uint64_t[max_iterations];

    // Create runner with deterministic timer
    benchmark::deterministic_timestamp_provider timer;
    benchmark::benchmark_runner runner(timing_buffer, max_iterations, &timer);

    // Configure benchmark
    benchmark::benchmark_config config{};
    config.warmup_iterations = 10;
    config.measured_iterations = 100;

    // Run benchmark
    benchmark::benchmark_result result = runner.run(bench, config);

    // Print results
    print_result(result);

    delete[] timing_buffer;
}

// Demonstrate regression detection
void demo_regression_detection() {
    printf("\n=== Regression Detection Demo ===\n");

    // Create baseline result
    benchmark::benchmark_result baseline{};
    baseline.name = "test_benchmark";
    baseline.workload_hash = 0x1234567890ABCDEFULL;
    baseline.timing.p99_ns = 1000;

    // Create current result (5% slower)
    benchmark::benchmark_result current = baseline;
    current.timing.p99_ns = 1050;

    // Compare results
    auto comparison = benchmark::compare_results(baseline, current, 0.05);

    printf("Baseline P99: %llu ns\n", static_cast<unsigned long long>(baseline.timing.p99_ns));
    printf("Current P99:  %llu ns\n", static_cast<unsigned long long>(current.timing.p99_ns));
    printf("Result: ");

    switch (comparison) {
        case benchmark::comparison_result::improved:
            printf("IMPROVED\n");
            break;
        case benchmark::comparison_result::unchanged:
            printf("UNCHANGED\n");
            break;
        case benchmark::comparison_result::regressed:
            printf("REGRESSED (Performance degradation detected!)\n");
            break;
        case benchmark::comparison_result::incomparable:
            printf("INCOMPARABLE\n");
            break;
    }
}

int main() {
    printf("╔════════════════════════════════════════════╗\n");
    printf("║  Aegis Deterministic Benchmark Harness     ║\n");
    printf("║  Measuring Correctness and Performance    ║\n");
    printf("╚════════════════════════════════════════════╝\n");

    // Run demos
    demo_workload_recording();
    run_frame_allocator_benchmark();
    run_frame_lifecycle_benchmark();
    demo_regression_detection();

    printf("\n=== Benchmark Demo Complete ===\n");
    printf("\nKey Features Demonstrated:\n");
    printf("  ✓ Deterministic workload recording and replay\n");
    printf("  ✓ P50 / P90 / P95 / P99 / P99.9 metrics\n");
    printf("  ✓ Regression detection with configurable thresholds\n");
    printf("  ✓ Workload hash verification for replay accuracy\n");
    printf("  ✓ Memory-efficient benchmark execution\n");
    printf("\n");

    return 0;
}
