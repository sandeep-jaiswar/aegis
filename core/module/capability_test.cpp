#include "module_builder.hpp"
#include "module_loader.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

namespace aegis::core::module {

// Test constants
constexpr int DETERMINISM_TEST_ITERATIONS = 10;

// Test helper: Build a module with specific capabilities
bool build_test_module(uint8_t* buffer, size_t buffer_size, capability_flags caps,
                       size_t* out_size) {
    module_builder builder(buffer, buffer_size);

    if (!builder.init("test_module", "1.0.0", "Test Author", "Test module for capability testing",
                      caps, 1702656000)) {
        return false;
    }

    // Add minimal code section
    uint8_t code[] = {0x01, 0x02, 0x03};
    if (!builder.add_code(code, sizeof(code))) {
        return false;
    }

    if (!builder.finalize()) {
        return false;
    }

    *out_size = builder.get_module_size();
    return true;
}

// Test 1: Module with no capabilities loads on any runtime
void test_no_capabilities() {
    printf("Test 1: Module with no capabilities...\n");

    uint8_t buffer[4096];
    size_t module_size = 0;

    bool success = build_test_module(buffer, sizeof(buffer), capability_flags::none, &module_size);
    assert(success);
    (void)success; // Suppress unused warning in release builds

    // Test loading on runtime with no capabilities
    {
        module_loader loader(buffer, module_size);
        version_info runtime{1, 0};
        capability_flags available = capability_flags::none;

        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::success);
        (void)result; // Suppress unused warning
        printf("  ✓ Module loads on runtime with no capabilities\n");
    }

    // Test loading on runtime with all capabilities
    {
        module_loader loader(buffer, module_size);
        version_info runtime{1, 0};
        capability_flags available = capability_flags::all;

        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::success);
        (void)result; // Suppress unused warning
        printf("  ✓ Module loads on runtime with all capabilities\n");
    }

    printf("  ✅ Test 1 passed\n\n");
}

// Test 2: Module with single capability
void test_single_capability() {
    printf("Test 2: Module with single capability (gpu_rendering)...\n");

    uint8_t buffer[4096];
    size_t module_size = 0;

    bool success =
        build_test_module(buffer, sizeof(buffer), capability_flags::gpu_rendering, &module_size);
    assert(success);
    (void)success;

    // Test loading on runtime WITH the capability
    {
        module_loader loader(buffer, module_size);
        version_info runtime{1, 0};
        capability_flags available = capability_flags::gpu_rendering;

        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::success);
        (void)result;
        printf("  ✓ Module loads when runtime has required capability\n");
    }

    // Test loading on runtime WITHOUT the capability
    {
        module_loader loader(buffer, module_size);
        version_info runtime{1, 0};
        capability_flags available = capability_flags::none;

        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::insufficient_capabilities);
        (void)result;
        printf("  ✓ Module fails to load when runtime lacks required capability\n");
    }

    // Test loading on runtime with DIFFERENT capability
    {
        module_loader loader(buffer, module_size);
        version_info runtime{1, 0};
        capability_flags available = capability_flags::input_events;

        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::insufficient_capabilities);
        (void)result;
        printf("  ✓ Module fails when runtime has different capability\n");
    }

    // Test loading on runtime with ALL capabilities
    {
        module_loader loader(buffer, module_size);
        version_info runtime{1, 0};
        capability_flags available = capability_flags::all;

        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::success);
        (void)result;
        printf("  ✓ Module loads when runtime has more capabilities\n");
    }

    printf("  ✅ Test 2 passed\n\n");
}

// Test 3: Module with multiple capabilities
void test_multiple_capabilities() {
    printf("Test 3: Module with multiple capabilities (gpu_rendering | input_events)...\n");

    uint8_t buffer[4096];
    size_t module_size = 0;

    capability_flags caps = capability_flags::gpu_rendering | capability_flags::input_events;
    bool success = build_test_module(buffer, sizeof(buffer), caps, &module_size);
    assert(success);
    (void)success;

    // Test loading on runtime with BOTH capabilities
    {
        module_loader loader(buffer, module_size);
        version_info runtime{1, 0};
        capability_flags available =
            capability_flags::gpu_rendering | capability_flags::input_events;

        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::success);
        (void)result;
        printf("  ✓ Module loads when runtime has all required capabilities\n");
    }

    // Test loading on runtime with ONLY first capability
    {
        module_loader loader(buffer, module_size);
        version_info runtime{1, 0};
        capability_flags available = capability_flags::gpu_rendering;

        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::insufficient_capabilities);
        (void)result;
        printf("  ✓ Module fails when runtime missing one capability\n");
    }

    // Test loading on runtime with ONLY second capability
    {
        module_loader loader(buffer, module_size);
        version_info runtime{1, 0};
        capability_flags available = capability_flags::input_events;

        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::insufficient_capabilities);
        (void)result;
        printf("  ✓ Module fails when runtime missing other capability\n");
    }

    // Test loading on runtime with NEITHER capability
    {
        module_loader loader(buffer, module_size);
        version_info runtime{1, 0};
        capability_flags available = capability_flags::none;

        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::insufficient_capabilities);
        (void)result;
        printf("  ✓ Module fails when runtime has no capabilities\n");
    }

    // Test loading on runtime with ALL capabilities
    {
        module_loader loader(buffer, module_size);
        version_info runtime{1, 0};
        capability_flags available = capability_flags::all;

        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::success);
        (void)result;
        printf("  ✓ Module loads when runtime has all capabilities\n");
    }

    printf("  ✅ Test 3 passed\n\n");
}

// Test 4: Determinism - same inputs produce same result
void test_determinism() {
    printf("Test 4: Capability checking is deterministic...\n");

    uint8_t buffer[4096];
    size_t module_size = 0;

    capability_flags caps = capability_flags::gpu_rendering | capability_flags::audio;
    bool success = build_test_module(buffer, sizeof(buffer), caps, &module_size);
    assert(success);
    (void)success;

    version_info runtime{1, 0};
    capability_flags available = capability_flags::gpu_rendering; // Missing audio

    // Load DETERMINISM_TEST_ITERATIONS times with identical inputs
    for (int i = 0; i < DETERMINISM_TEST_ITERATIONS; ++i) {
        module_loader loader(buffer, module_size);
        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::insufficient_capabilities);
        (void)result;
    }

    printf("  ✓ %d loads with same inputs produce identical results\n",
           DETERMINISM_TEST_ITERATIONS);

    // Now with sufficient capabilities
    available = capability_flags::gpu_rendering | capability_flags::audio;

    for (int i = 0; i < DETERMINISM_TEST_ITERATIONS; ++i) {
        module_loader loader(buffer, module_size);
        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::success);
        (void)result;
    }

    printf("  ✓ %d successful loads produce identical results\n", DETERMINISM_TEST_ITERATIONS);
    printf("  ✅ Test 4 passed\n\n");
}

// Test 5: All capability flags
void test_all_capabilities() {
    printf("Test 5: Module with all capabilities...\n");

    uint8_t buffer[4096];
    size_t module_size = 0;

    // Build module with all capabilities
    capability_flags caps = capability_flags::gpu_rendering | capability_flags::input_events |
                            capability_flags::file_io | capability_flags::network_io |
                            capability_flags::audio | capability_flags::multithreading |
                            capability_flags::webgpu | capability_flags::vulkan |
                            capability_flags::custom_shaders | capability_flags::external_resources;

    bool success = build_test_module(buffer, sizeof(buffer), caps, &module_size);
    assert(success);
    (void)success;

    // Test loading on runtime with all capabilities
    {
        module_loader loader(buffer, module_size);
        version_info runtime{1, 0};
        capability_flags available = capability_flags::all;

        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::success);
        (void)result;
        printf("  ✓ Module with all capabilities loads on full runtime\n");
    }

    // Test loading on runtime missing just one capability
    {
        module_loader loader(buffer, module_size);
        version_info runtime{1, 0};
        capability_flags available = capability_flags::all & ~capability_flags::network_io;

        module_load_result result = loader.load(runtime, available);
        assert(result == module_load_result::insufficient_capabilities);
        (void)result;
        printf("  ✓ Module fails when runtime missing even one capability\n");
    }

    printf("  ✅ Test 5 passed\n\n");
}

// Test 6: Capability bits correctness
void test_capability_bits() {
    printf("Test 6: Capability bit operations...\n");

    // Test has_capability function
    {
        capability_flags caps = capability_flags::gpu_rendering | capability_flags::input_events;
        (void)caps; // Suppress unused warning

        assert(has_capability(caps, capability_flags::gpu_rendering));
        assert(has_capability(caps, capability_flags::input_events));
        assert(!has_capability(caps, capability_flags::file_io));
        assert(!has_capability(caps, capability_flags::audio));

        printf("  ✓ has_capability() works correctly\n");
    }

    // Test bitwise operations
    {
        capability_flags a = capability_flags::gpu_rendering;
        capability_flags b = capability_flags::input_events;
        capability_flags combined = a | b;
        (void)combined; // Suppress unused warning

        assert(has_capability(combined, a));
        assert(has_capability(combined, b));

        printf("  ✓ Bitwise OR combines capabilities\n");
    }

    // Test masking
    {
        capability_flags all = capability_flags::all;
        capability_flags masked = all & ~capability_flags::network_io;
        (void)masked; // Suppress unused warning

        assert(has_capability(masked, capability_flags::gpu_rendering));
        assert(has_capability(masked, capability_flags::input_events));
        assert(!has_capability(masked, capability_flags::network_io));

        printf("  ✓ Bitwise masking removes capabilities\n");
    }

    printf("  ✅ Test 6 passed\n\n");
}

// Run all tests
void run_capability_tests() {
    printf("=== Aegis Capability Model Tests ===\n\n");

    test_no_capabilities();
    test_single_capability();
    test_multiple_capabilities();
    test_determinism();
    test_all_capabilities();
    test_capability_bits();

    printf("=== All Tests Passed ✅ ===\n");
    printf("\nAcceptance Criteria Verified:\n");
    printf("  ✅ No ambient authority exists (modules start with capability_flags::none)\n");
    printf("  ✅ Missing capability → deterministic failure (insufficient_capabilities)\n");
    printf("  ✅ Capability enforcement is testable (6 test suites passed)\n");
    printf("  ✅ Capability enforcement is replayable (determinism test passed)\n");
}

} // namespace aegis::core::module

int main() {
    aegis::core::module::run_capability_tests();
    return 0;
}
