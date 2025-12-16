// Runtime Conformance Test Suite
// Ticket: REF-002
//
// This header defines tests that any Aegis runtime must pass to be conformant.
// Tests are implementation-agnostic and verify the core runtime invariants.
//
// Test Categories:
// 1. Determinism tests - Same inputs → same outputs
// 2. Event replay tests - Record/replay yields identical results
// 3. Memory behavior tests - Allocation patterns are deterministic
// 4. Capability enforcement tests - Missing capabilities → deterministic failure
//
// Acceptance Criteria:
// ✅ Tests are implementation-agnostic
// ✅ Failures clearly attribute violated invariant
// ✅ Clean-room runtime can validate against suite

#pragma once

#include <cstddef>
#include <cstdint>

namespace aegis::runtime::conformance {

// Test result codes
enum class test_result : uint8_t {
    success = 0,           // Test passed
    failure = 1,           // Test failed (assertion violation)
    skipped = 2,           // Test skipped (not applicable)
    not_implemented = 3,   // Feature not implemented by runtime
    error = 4              // Test error (infrastructure issue)
};

// Test statistics
struct test_stats {
    size_t total_tests{0};
    size_t passed{0};
    size_t failed{0};
    size_t skipped{0};
    size_t not_implemented{0};
    size_t errors{0};
};

// Test case metadata
struct test_case {
    const char* name;
    const char* description;
    const char* invariant;  // What invariant this test verifies
    test_result (*test_fn)() noexcept;
};

// ====================================================================
// DETERMINISM TESTS
// ====================================================================

// Test: Frame execution is deterministic
// Invariant: Same inputs → same frame stats
test_result test_frame_determinism() noexcept;

// Test: Event ordering is deterministic
// Invariant: Events processed in timestamp order, tie-broken by sequence
test_result test_event_ordering_determinism() noexcept;

// Test: Memory allocation is deterministic
// Invariant: Same inputs → same allocation pattern
test_result test_memory_allocation_determinism() noexcept;

// Test: Floating-point operations are deterministic
// Invariant: IEEE 754 compliance, no fast-math
test_result test_floating_point_determinism() noexcept;

// Test: Workload hash is deterministic
// Invariant: Same workload → same hash
test_result test_workload_hash_determinism() noexcept;

// ====================================================================
// EVENT REPLAY TESTS
// ====================================================================

// Test: Event recording captures all inputs
// Invariant: Record → replay produces same input_event sequence
test_result test_event_recording() noexcept;

// Test: Event replay produces identical output
// Invariant: Record run hash == replay run hash
test_result test_event_replay_determinism() noexcept;

// Test: Workload can be serialized and deserialized
// Invariant: serialize(w) → deserialize → w' where w == w'
test_result test_workload_serialization() noexcept;

// Test: Cross-run replay verification
// Invariant: Kill runtime, replay log → identical output
test_result test_cross_run_replay() noexcept;

// ====================================================================
// MEMORY BEHAVIOR TESTS
// ====================================================================

// Test: Frame allocator resets every frame
// Invariant: Frame N allocation == Frame N+k allocation (for same inputs)
test_result test_frame_allocator_reset() noexcept;

// Test: Arena allocator is deterministic
// Invariant: Same allocation sequence → same memory layout
test_result test_arena_allocator_determinism() noexcept;

// Test: Pool allocator is deterministic
// Invariant: Same alloc/free sequence → same allocations
test_result test_pool_allocator_determinism() noexcept;

// Test: No memory leaks in deterministic path
// Invariant: After N frames, allocated == deallocated
test_result test_memory_leak_detection() noexcept;

// Test: Memory usage is bounded
// Invariant: Peak memory ≤ configured limit
test_result test_memory_usage_bounds() noexcept;

// ====================================================================
// CAPABILITY ENFORCEMENT TESTS
// ====================================================================

// Test: Module with no capabilities loads
// Invariant: capability_flags::none → module_load_result::success
test_result test_no_capabilities_loads() noexcept;

// Test: Module with missing capability fails
// Invariant: required ∉ available → module_load_result::insufficient_capabilities
test_result test_missing_capability_fails() noexcept;

// Test: Module with all capabilities loads
// Invariant: required ⊆ available → module_load_result::success
test_result test_all_capabilities_loads() noexcept;

// Test: Capability check is deterministic
// Invariant: Same module + same runtime → same load result
test_result test_capability_check_determinism() noexcept;

// Test: Capability enforcement at runtime
// Invariant: Missing capability → deterministic failure at feature access
test_result test_runtime_capability_enforcement() noexcept;

// ====================================================================
// CROSS-PLATFORM VERIFICATION TESTS
// ====================================================================

// Test: Same workload on different platforms
// Invariant: Linux hash == macOS hash == Windows hash
test_result test_cross_platform_determinism() noexcept;

// Test: Same workload with different compilers
// Invariant: GCC hash == Clang hash == MSVC hash
test_result test_cross_compiler_determinism() noexcept;

// Test: Same workload at different optimizations
// Invariant: Debug hash == Release hash (for deterministic code)
test_result test_optimization_level_determinism() noexcept;

// ====================================================================
// TIMING TESTS
// ====================================================================

// Test: Timestamps are monotonic
// Invariant: t[i] ≤ t[i+1]
test_result test_timestamp_monotonicity() noexcept;

// Test: Timestamps are deterministic during replay
// Invariant: Recorded timestamps == replayed timestamps
test_result test_timestamp_replay_determinism() noexcept;

// Test: Frame timing is deterministic
// Invariant: Same inputs → same phase durations
test_result test_frame_timing_determinism() noexcept;

// ====================================================================
// MODULE LOADING TESTS
// ====================================================================

// Test: Valid module loads successfully
// Invariant: Well-formed module → module_load_result::success
test_result test_valid_module_loads() noexcept;

// Test: Invalid magic fails deterministically
// Invariant: Bad magic → module_load_result::invalid_magic
test_result test_invalid_magic_fails() noexcept;

// Test: Version mismatch fails deterministically
// Invariant: Incompatible version → module_load_result::unsupported_version
test_result test_version_mismatch_fails() noexcept;

// Test: Hash mismatch fails deterministically
// Invariant: Corrupted data → module_load_result::hash_mismatch
test_result test_hash_mismatch_fails() noexcept;

// ====================================================================
// TEST SUITE RUNNER
// ====================================================================

// Run all conformance tests
// Returns test statistics
test_stats run_conformance_tests() noexcept;

// Run specific test category
test_stats run_determinism_tests() noexcept;
test_stats run_replay_tests() noexcept;
test_stats run_memory_tests() noexcept;
test_stats run_capability_tests() noexcept;

// Print test results
void print_test_stats(const test_stats& stats) noexcept;

// ====================================================================
// ACCEPTANCE CRITERIA VERIFICATION
// ====================================================================

// REF-002 Acceptance Criteria:
//
// ✅ Tests are implementation-agnostic
//    - All tests use public core/ APIs
//    - No dependency on runtime implementation details
//    - Can be run against any conforming runtime
//
// ✅ Failures clearly attribute violated invariant
//    - Each test specifies the invariant it verifies
//    - Test output shows which invariant was violated
//    - Clear failure messages for debugging
//
// ✅ Clean-room runtime can validate against suite
//    - Tests define required behavior
//    - No assumptions about implementation
//    - Independent reimplementation can use same tests

} // namespace aegis::runtime::conformance
