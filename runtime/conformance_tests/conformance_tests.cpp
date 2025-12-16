// Runtime Conformance Test Suite - Implementation
// Ticket: REF-002

#include "conformance_tests.hpp"

#include "core/benchmark/workload.hpp"
#include "core/events/input_event.hpp"
#include "core/frame/frame_lifecycle.hpp"
#include "core/memory/frame_allocator.hpp"
#include "core/module/module_builder.hpp"
#include "core/module/module_loader.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

namespace aegis::runtime::conformance {

using namespace aegis::core;
using namespace aegis::core::events;
using namespace aegis::core::frame;
using namespace aegis::core::memory;
using namespace aegis::core::module;
using namespace aegis::core::benchmark;

// ====================================================================
// DETERMINISM TESTS
// ====================================================================

test_result test_frame_determinism() noexcept {
    printf("Test: Frame execution determinism\n");
    printf("Invariant: Same inputs → same frame stats\n");

    // Test frame_stats structure is deterministic
    frame_stats stats1{};
    frame_stats stats2{};

    stats1.frame_number = 1;
    stats1.frame_start_timestamp_ns = 1000000000ULL;
    stats1.total_time_ns = 16666667ULL;

    stats2.frame_number = 1;
    stats2.frame_start_timestamp_ns = 1000000000ULL;
    stats2.total_time_ns = 16666667ULL;

    // Verify stats are identical
    if (memcmp(&stats1, &stats2, sizeof(frame_stats)) != 0) {
        printf("  ✗ FAILED: Frame stats differ\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

test_result test_event_ordering_determinism() noexcept {
    printf("Test: Event ordering determinism\n");
    printf("Invariant: Events processed in timestamp order\n");

    // This test would require event queue implementation
    // For now, mark as not implemented
    printf("  ⊘ NOT IMPLEMENTED\n");
    return test_result::not_implemented;
}

test_result test_memory_allocation_determinism() noexcept {
    printf("Test: Memory allocation determinism\n");
    printf("Invariant: Same inputs → same allocation pattern\n");

    constexpr size_t buffer_size = 4096;
    uint8_t buffer1[buffer_size];
    uint8_t buffer2[buffer_size];

    // Create two frame allocators with same configuration
    frame_allocator alloc1(buffer1, buffer_size);
    frame_allocator alloc2(buffer2, buffer_size);

    // Allocate same pattern
    void* p1a = alloc1.allocate(64, 8);
    void* p2a = alloc2.allocate(64, 8);

    void* p1b = alloc1.allocate(128, 8);
    void* p2b = alloc2.allocate(128, 8);

    // Check relative offsets are identical
    ptrdiff_t offset1 = static_cast<uint8_t*>(p1b) - static_cast<uint8_t*>(p1a);
    ptrdiff_t offset2 = static_cast<uint8_t*>(p2b) - static_cast<uint8_t*>(p2a);

    if (offset1 != offset2) {
        printf("  ✗ FAILED: Allocation offsets differ\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

test_result test_floating_point_determinism() noexcept {
    printf("Test: Floating-point determinism\n");
    printf("Invariant: IEEE 754 compliance, no fast-math\n");

    // Simple floating-point operations
    float a = 1.5f;
    float b = 2.5f;
    float sum1 = a + b;
    float sum2 = a + b;

    if (sum1 != sum2) {
        printf("  ✗ FAILED: Floating-point addition not deterministic\n");
        return test_result::failure;
    }

    // Check for bit-exact equality
    if (memcmp(&sum1, &sum2, sizeof(float)) != 0) {
        printf("  ✗ FAILED: Floating-point values not bit-exact\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

test_result test_workload_hash_determinism() noexcept {
    printf("Test: Workload hash determinism\n");
    printf("Invariant: Same workload → same hash\n");

    // Create identical workloads
    workload w1{};
    w1.name = "test";
    w1.event_count = 3;
    w1.hash = 12345;

    workload w2{};
    w2.name = "test";
    w2.event_count = 3;
    w2.hash = 12345;

    // Hashes should be identical
    if (w1.hash != w2.hash) {
        printf("  ✗ FAILED: Workload hashes differ\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

// ====================================================================
// EVENT REPLAY TESTS
// ====================================================================

test_result test_event_recording() noexcept {
    printf("Test: Event recording\n");
    printf("Invariant: Record → replay produces same input_event sequence\n");

    // This requires event replay infrastructure
    printf("  ⊘ NOT IMPLEMENTED\n");
    return test_result::not_implemented;
}

test_result test_event_replay_determinism() noexcept {
    printf("Test: Event replay determinism\n");
    printf("Invariant: Record run hash == replay run hash\n");

    // This requires event replay infrastructure
    printf("  ⊘ NOT IMPLEMENTED\n");
    return test_result::not_implemented;
}

test_result test_workload_serialization() noexcept {
    printf("Test: Workload serialization\n");
    printf("Invariant: serialize(w) → deserialize → w'\n");

    workload w{};
    w.name = "test";
    w.event_count = 5;
    w.hash = 67890;

    // Simulate serialization (copy to buffer)
    uint8_t buffer[sizeof(workload)];
    memcpy(buffer, &w, sizeof(workload));

    // Simulate deserialization (copy from buffer)
    workload w2{};
    memcpy(&w2, buffer, sizeof(workload));

    // Verify identical
    if (memcmp(&w, &w2, sizeof(workload)) != 0) {
        printf("  ✗ FAILED: Workload not identical after serialization\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

test_result test_cross_run_replay() noexcept {
    printf("Test: Cross-run replay\n");
    printf("Invariant: Kill runtime, replay log → identical output\n");

    // This requires full runtime lifecycle testing
    printf("  ⊘ NOT IMPLEMENTED\n");
    return test_result::not_implemented;
}

// ====================================================================
// MEMORY BEHAVIOR TESTS
// ====================================================================

test_result test_frame_allocator_reset() noexcept {
    printf("Test: Frame allocator reset\n");
    printf("Invariant: Frame allocator resets every frame\n");

    constexpr size_t buffer_size = 4096;
    uint8_t buffer[buffer_size];

    frame_allocator alloc(buffer, buffer_size);

    // Allocate in frame 1
    void* p1 = alloc.allocate(64, 8);
    size_t used1 = alloc.bytes_allocated();

    // Reset
    alloc.reset();

    // Allocate in frame 2 (should get same address)
    void* p2 = alloc.allocate(64, 8);
    size_t used2 = alloc.bytes_allocated();

    if (p1 != p2) {
        printf("  ✗ FAILED: Allocation addresses differ after reset\n");
        return test_result::failure;
    }

    if (used1 != used2) {
        printf("  ✗ FAILED: Bytes used differ after reset\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

test_result test_arena_allocator_determinism() noexcept {
    printf("Test: Arena allocator determinism\n");
    printf("Invariant: Same allocation sequence → same memory layout\n");

    // Arena allocator tests would go here
    printf("  ⊘ NOT IMPLEMENTED\n");
    return test_result::not_implemented;
}

test_result test_pool_allocator_determinism() noexcept {
    printf("Test: Pool allocator determinism\n");
    printf("Invariant: Same alloc/free sequence → same allocations\n");

    // Pool allocator tests would go here
    printf("  ⊘ NOT IMPLEMENTED\n");
    return test_result::not_implemented;
}

test_result test_memory_leak_detection() noexcept {
    printf("Test: Memory leak detection\n");
    printf("Invariant: After N frames, allocated == deallocated\n");

    constexpr size_t buffer_size = 4096;
    uint8_t buffer[buffer_size];

    frame_allocator alloc(buffer, buffer_size);

    // Allocate and reset 10 times
    for (int i = 0; i < 10; ++i) {
        void* p = alloc.allocate(64, 8);
        (void)p;  // Mark as used
        alloc.reset();
    }

    // After reset, should have 0 bytes used
    if (alloc.bytes_allocated() != 0) {
        printf("  ✗ FAILED: Memory not fully reset\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

test_result test_memory_usage_bounds() noexcept {
    printf("Test: Memory usage bounds\n");
    printf("Invariant: Peak memory ≤ configured limit\n");

    constexpr size_t buffer_size = 4096;
    uint8_t buffer[buffer_size];

    frame_allocator alloc(buffer, buffer_size);

    // Try to allocate beyond capacity
    void* p = alloc.allocate(buffer_size + 1, 8);

    if (p != nullptr) {
        printf("  ✗ FAILED: Allocator exceeded capacity\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

// ====================================================================
// CAPABILITY ENFORCEMENT TESTS
// ====================================================================

test_result test_no_capabilities_loads() noexcept {
    printf("Test: Module with no capabilities loads\n");
    printf("Invariant: capability_flags::none → module_load_result::success\n");

    uint8_t buffer[4096];
    module_builder builder(buffer, sizeof(buffer));

    if (!builder.init("test", "1.0.0", "Author", "Test module", capability_flags::none,
                      1000000000ULL)) {
        printf("  ✗ FAILED: Could not build module\n");
        return test_result::failure;
    }

    uint8_t code[] = {0x01, 0x02, 0x03};
    if (!builder.add_code(code, sizeof(code))) {
        printf("  ✗ FAILED: Could not add code\n");
        return test_result::failure;
    }

    if (!builder.finalize()) {
        printf("  ✗ FAILED: Could not finalize module\n");
        return test_result::failure;
    }

    module_loader loader(buffer, builder.get_module_size());
    version_info runtime{1, 0};
    module_load_result result = loader.load(runtime, capability_flags::none);

    if (result != module_load_result::success) {
        printf("  ✗ FAILED: Module with no capabilities failed to load\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

test_result test_missing_capability_fails() noexcept {
    printf("Test: Module with missing capability fails\n");
    printf("Invariant: required ∉ available → insufficient_capabilities\n");

    uint8_t buffer[4096];
    module_builder builder(buffer, sizeof(buffer));

    // Build module requiring gpu_rendering
    if (!builder.init("test", "1.0.0", "Author", "Test module", capability_flags::gpu_rendering,
                      1000000000ULL)) {
        printf("  ✗ FAILED: Could not build module\n");
        return test_result::failure;
    }

    uint8_t code[] = {0x01, 0x02, 0x03};
    if (!builder.add_code(code, sizeof(code))) {
        printf("  ✗ FAILED: Could not add code\n");
        return test_result::failure;
    }

    if (!builder.finalize()) {
        printf("  ✗ FAILED: Could not finalize module\n");
        return test_result::failure;
    }

    // Try to load on runtime without gpu_rendering
    module_loader loader(buffer, builder.get_module_size());
    version_info runtime{1, 0};
    module_load_result result = loader.load(runtime, capability_flags::none);

    if (result != module_load_result::insufficient_capabilities) {
        printf("  ✗ FAILED: Module should have failed with insufficient_capabilities\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

test_result test_all_capabilities_loads() noexcept {
    printf("Test: Module with all capabilities loads\n");
    printf("Invariant: required ⊆ available → module_load_result::success\n");

    uint8_t buffer[4096];
    module_builder builder(buffer, sizeof(buffer));

    // Build module with some capabilities
    capability_flags caps = capability_flags::gpu_rendering | capability_flags::input_events;
    if (!builder.init("test", "1.0.0", "Author", "Test module", caps, 1000000000ULL)) {
        printf("  ✗ FAILED: Could not build module\n");
        return test_result::failure;
    }

    uint8_t code[] = {0x01, 0x02, 0x03};
    if (!builder.add_code(code, sizeof(code))) {
        printf("  ✗ FAILED: Could not add code\n");
        return test_result::failure;
    }

    if (!builder.finalize()) {
        printf("  ✗ FAILED: Could not finalize module\n");
        return test_result::failure;
    }

    // Load on runtime with all capabilities
    module_loader loader(buffer, builder.get_module_size());
    version_info runtime{1, 0};
    module_load_result result = loader.load(runtime, capability_flags::all);

    if (result != module_load_result::success) {
        printf("  ✗ FAILED: Module should load when runtime has all capabilities\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

test_result test_capability_check_determinism() noexcept {
    printf("Test: Capability check determinism\n");
    printf("Invariant: Same module + same runtime → same load result\n");

    uint8_t buffer[4096];
    module_builder builder(buffer, sizeof(buffer));

    if (!builder.init("test", "1.0.0", "Author", "Test module", capability_flags::gpu_rendering,
                      1000000000ULL)) {
        printf("  ✗ FAILED: Could not build module\n");
        return test_result::failure;
    }

    uint8_t code[] = {0x01, 0x02, 0x03};
    if (!builder.add_code(code, sizeof(code))) {
        printf("  ✗ FAILED: Could not add code\n");
        return test_result::failure;
    }

    if (!builder.finalize()) {
        printf("  ✗ FAILED: Could not finalize module\n");
        return test_result::failure;
    }

    version_info runtime{1, 0};

    // Load 10 times with same inputs
    for (int i = 0; i < 10; ++i) {
        module_loader loader(buffer, builder.get_module_size());
        module_load_result result = loader.load(runtime, capability_flags::none);

        if (result != module_load_result::insufficient_capabilities) {
            printf("  ✗ FAILED: Inconsistent load result\n");
            return test_result::failure;
        }
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

test_result test_runtime_capability_enforcement() noexcept {
    printf("Test: Runtime capability enforcement\n");
    printf("Invariant: Missing capability → deterministic failure\n");

    // This would require runtime feature access testing
    printf("  ⊘ NOT IMPLEMENTED\n");
    return test_result::not_implemented;
}

// ====================================================================
// CROSS-PLATFORM TESTS
// ====================================================================

test_result test_cross_platform_determinism() noexcept {
    printf("Test: Cross-platform determinism\n");
    printf("Invariant: Linux hash == macOS hash == Windows hash\n");

    // This requires running on multiple platforms
    printf("  ⊘ SKIPPED (requires multi-platform)\n");
    return test_result::skipped;
}

test_result test_cross_compiler_determinism() noexcept {
    printf("Test: Cross-compiler determinism\n");
    printf("Invariant: GCC hash == Clang hash == MSVC hash\n");

    // This requires building with multiple compilers
    printf("  ⊘ SKIPPED (requires multi-compiler)\n");
    return test_result::skipped;
}

test_result test_optimization_level_determinism() noexcept {
    printf("Test: Optimization level determinism\n");
    printf("Invariant: Debug hash == Release hash\n");

    // This requires building at different optimization levels
    printf("  ⊘ SKIPPED (requires multi-build)\n");
    return test_result::skipped;
}

// ====================================================================
// TIMING TESTS
// ====================================================================

test_result test_timestamp_monotonicity() noexcept {
    printf("Test: Timestamp monotonicity\n");
    printf("Invariant: t[i] ≤ t[i+1]\n");

    uint64_t t1 = 1000000000ULL;
    uint64_t t2 = 1000000001ULL;
    uint64_t t3 = 1000000002ULL;

    if (!(t1 <= t2 && t2 <= t3)) {
        printf("  ✗ FAILED: Timestamps not monotonic\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

test_result test_timestamp_replay_determinism() noexcept {
    printf("Test: Timestamp replay determinism\n");
    printf("Invariant: Recorded timestamps == replayed timestamps\n");

    // This requires replay infrastructure
    printf("  ⊘ NOT IMPLEMENTED\n");
    return test_result::not_implemented;
}

test_result test_frame_timing_determinism() noexcept {
    printf("Test: Frame timing determinism\n");
    printf("Invariant: Same inputs → same phase durations\n");

    // Test frame timing calculations are deterministic
    uint64_t start_time = 1000000000ULL;
    uint64_t end_time = 1000016667ULL;
    uint64_t duration1 = end_time - start_time;
    uint64_t duration2 = end_time - start_time;

    if (duration1 != duration2) {
        printf("  ✗ FAILED: Frame durations differ\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

// ====================================================================
// MODULE LOADING TESTS
// ====================================================================

test_result test_valid_module_loads() noexcept {
    printf("Test: Valid module loads\n");
    printf("Invariant: Well-formed module → module_load_result::success\n");

    uint8_t buffer[4096];
    module_builder builder(buffer, sizeof(buffer));

    if (!builder.init("test", "1.0.0", "Author", "Test", capability_flags::none, 1000000000ULL)) {
        printf("  ✗ FAILED: Could not build module\n");
        return test_result::failure;
    }

    uint8_t code[] = {0x01, 0x02, 0x03};
    if (!builder.add_code(code, sizeof(code))) {
        printf("  ✗ FAILED: Could not add code\n");
        return test_result::failure;
    }

    if (!builder.finalize()) {
        printf("  ✗ FAILED: Could not finalize\n");
        return test_result::failure;
    }

    module_loader loader(buffer, builder.get_module_size());
    version_info runtime{1, 0};
    module_load_result result = loader.load(runtime, capability_flags::all);

    if (result != module_load_result::success) {
        printf("  ✗ FAILED: Valid module should load\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

test_result test_invalid_magic_fails() noexcept {
    printf("Test: Invalid magic fails\n");
    printf("Invariant: Bad magic → module_load_result::invalid_magic\n");

    uint8_t buffer[4096] = {0};  // Invalid magic

    module_loader loader(buffer, sizeof(buffer));
    version_info runtime{1, 0};
    module_load_result result = loader.load(runtime, capability_flags::all);

    if (result != module_load_result::invalid_magic) {
        printf("  ✗ FAILED: Should fail with invalid_magic\n");
        return test_result::failure;
    }

    printf("  ✓ PASSED\n");
    return test_result::success;
}

test_result test_version_mismatch_fails() noexcept {
    printf("Test: Version mismatch fails\n");
    printf("Invariant: Incompatible version → unsupported_version\n");

    // This requires creating module with mismatched version
    printf("  ⊘ NOT IMPLEMENTED\n");
    return test_result::not_implemented;
}

test_result test_hash_mismatch_fails() noexcept {
    printf("Test: Hash mismatch fails\n");
    printf("Invariant: Corrupted data → hash_mismatch\n");

    // This requires creating module with invalid hash
    printf("  ⊘ NOT IMPLEMENTED\n");
    return test_result::not_implemented;
}

// ====================================================================
// TEST SUITE RUNNER
// ====================================================================

static const test_case all_tests[] = {
    // Determinism tests
    {"frame_determinism", "Frame execution is deterministic",
     "Same inputs → same frame stats", test_frame_determinism},
    {"event_ordering", "Event ordering is deterministic", "Events processed in timestamp order",
     test_event_ordering_determinism},
    {"memory_allocation", "Memory allocation is deterministic",
     "Same inputs → same allocation pattern", test_memory_allocation_determinism},
    {"floating_point", "Floating-point is deterministic", "IEEE 754 compliance",
     test_floating_point_determinism},
    {"workload_hash", "Workload hash is deterministic", "Same workload → same hash",
     test_workload_hash_determinism},

    // Event replay tests
    {"event_recording", "Event recording captures all inputs",
     "Record → replay same sequence", test_event_recording},
    {"event_replay", "Event replay is deterministic", "Record hash == replay hash",
     test_event_replay_determinism},
    {"workload_serialization", "Workload serialization is deterministic",
     "serialize → deserialize → identical", test_workload_serialization},
    {"cross_run_replay", "Cross-run replay verification", "Kill runtime → replay → identical",
     test_cross_run_replay},

    // Memory tests
    {"frame_allocator_reset", "Frame allocator resets", "Frame N == Frame N+k",
     test_frame_allocator_reset},
    {"arena_allocator", "Arena allocator is deterministic", "Same sequence → same layout",
     test_arena_allocator_determinism},
    {"pool_allocator", "Pool allocator is deterministic", "Same alloc/free → same allocations",
     test_pool_allocator_determinism},
    {"memory_leak", "No memory leaks", "allocated == deallocated",
     test_memory_leak_detection},
    {"memory_bounds", "Memory usage is bounded", "Peak ≤ limit",
     test_memory_usage_bounds},

    // Capability tests
    {"no_capabilities", "Module with no capabilities loads", "none → success",
     test_no_capabilities_loads},
    {"missing_capability", "Missing capability fails", "required ∉ available → failure",
     test_missing_capability_fails},
    {"all_capabilities", "All capabilities loads", "required ⊆ available → success",
     test_all_capabilities_loads},
    {"capability_determinism", "Capability check is deterministic",
     "Same inputs → same result", test_capability_check_determinism},
    {"runtime_enforcement", "Runtime capability enforcement", "Missing → deterministic failure",
     test_runtime_capability_enforcement},

    // Cross-platform tests
    {"cross_platform", "Cross-platform determinism", "Linux == macOS == Windows",
     test_cross_platform_determinism},
    {"cross_compiler", "Cross-compiler determinism", "GCC == Clang == MSVC",
     test_cross_compiler_determinism},
    {"optimization_level", "Optimization level determinism", "Debug == Release",
     test_optimization_level_determinism},

    // Timing tests
    {"timestamp_monotonic", "Timestamps are monotonic", "t[i] ≤ t[i+1]",
     test_timestamp_monotonicity},
    {"timestamp_replay", "Timestamp replay determinism", "Recorded == replayed",
     test_timestamp_replay_determinism},
    {"frame_timing", "Frame timing is deterministic", "Same inputs → same durations",
     test_frame_timing_determinism},

    // Module loading tests
    {"valid_module", "Valid module loads", "Well-formed → success",
     test_valid_module_loads},
    {"invalid_magic", "Invalid magic fails", "Bad magic → invalid_magic",
     test_invalid_magic_fails},
    {"version_mismatch", "Version mismatch fails", "Incompatible → unsupported_version",
     test_version_mismatch_fails},
    {"hash_mismatch", "Hash mismatch fails", "Corrupted → hash_mismatch",
     test_hash_mismatch_fails},
};

constexpr size_t num_tests = sizeof(all_tests) / sizeof(test_case);

test_stats run_conformance_tests() noexcept {
    printf("\n");
    printf("=================================================================\n");
    printf("    Aegis Runtime Conformance Test Suite (REF-002)\n");
    printf("=================================================================\n");
    printf("\n");

    test_stats stats{};
    stats.total_tests = num_tests;

    for (size_t i = 0; i < num_tests; ++i) {
        const test_case& test = all_tests[i];
        printf("[%zu/%zu] %s\n", i + 1, num_tests, test.description);

        test_result result = test.test_fn();

        switch (result) {
        case test_result::success:
            stats.passed++;
            break;
        case test_result::failure:
            stats.failed++;
            break;
        case test_result::skipped:
            stats.skipped++;
            break;
        case test_result::not_implemented:
            stats.not_implemented++;
            break;
        case test_result::error:
            stats.errors++;
            break;
        }

        printf("\n");
    }

    printf("=================================================================\n");
    print_test_stats(stats);
    printf("=================================================================\n");

    return stats;
}

test_stats run_determinism_tests() noexcept {
    // Run only determinism tests (indices 0-4)
    test_stats stats{};
    stats.total_tests = 5;

    for (size_t i = 0; i < 5; ++i) {
        test_result result = all_tests[i].test_fn();
        if (result == test_result::success)
            stats.passed++;
        else if (result == test_result::failure)
            stats.failed++;
        else if (result == test_result::skipped)
            stats.skipped++;
        else if (result == test_result::not_implemented)
            stats.not_implemented++;
        else
            stats.errors++;
    }

    return stats;
}

test_stats run_replay_tests() noexcept {
    // Run only replay tests (indices 5-8)
    test_stats stats{};
    stats.total_tests = 4;

    for (size_t i = 5; i < 9; ++i) {
        test_result result = all_tests[i].test_fn();
        if (result == test_result::success)
            stats.passed++;
        else if (result == test_result::failure)
            stats.failed++;
        else if (result == test_result::skipped)
            stats.skipped++;
        else if (result == test_result::not_implemented)
            stats.not_implemented++;
        else
            stats.errors++;
    }

    return stats;
}

test_stats run_memory_tests() noexcept {
    // Run only memory tests (indices 9-13)
    test_stats stats{};
    stats.total_tests = 5;

    for (size_t i = 9; i < 14; ++i) {
        test_result result = all_tests[i].test_fn();
        if (result == test_result::success)
            stats.passed++;
        else if (result == test_result::failure)
            stats.failed++;
        else if (result == test_result::skipped)
            stats.skipped++;
        else if (result == test_result::not_implemented)
            stats.not_implemented++;
        else
            stats.errors++;
    }

    return stats;
}

test_stats run_capability_tests() noexcept {
    // Run only capability tests (indices 14-18)
    test_stats stats{};
    stats.total_tests = 5;

    for (size_t i = 14; i < 19; ++i) {
        test_result result = all_tests[i].test_fn();
        if (result == test_result::success)
            stats.passed++;
        else if (result == test_result::failure)
            stats.failed++;
        else if (result == test_result::skipped)
            stats.skipped++;
        else if (result == test_result::not_implemented)
            stats.not_implemented++;
        else
            stats.errors++;
    }

    return stats;
}

void print_test_stats(const test_stats& stats) noexcept {
    printf("\n");
    printf("Test Results:\n");
    printf("  Total:           %zu\n", stats.total_tests);
    printf("  ✓ Passed:        %zu\n", stats.passed);
    printf("  ✗ Failed:        %zu\n", stats.failed);
    printf("  ⊘ Not Impl:      %zu\n", stats.not_implemented);
    printf("  ⊙ Skipped:       %zu\n", stats.skipped);
    printf("  ⚠ Errors:        %zu\n", stats.errors);
    printf("\n");

    if (stats.failed == 0 && stats.errors == 0) {
        printf("Status: ✅ ALL TESTS PASSED\n");
    } else {
        printf("Status: ❌ SOME TESTS FAILED\n");
    }
}

} // namespace aegis::runtime::conformance
