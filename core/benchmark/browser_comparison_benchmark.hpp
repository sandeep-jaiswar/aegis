#pragma once

#include "core/benchmark/benchmark.hpp"
#include "core/memory/frame_allocator.hpp"

#include <cstddef>
#include <cstdint>

namespace aegis::core::benchmark {

// Browser comparison workload configuration
// These match the JavaScript implementation in browser_comparison.html
struct browser_comparison_config {
    static constexpr size_t allocations_per_frame = 50;
    static constexpr size_t operations_per_frame = 100;
    static constexpr size_t allocation_sizes[] = {64, 128, 256, 512, 1024};
    static constexpr size_t allocation_size_count = 5;
};

// Browser comparison benchmark
// This benchmark replicates the exact workload performed by the browser-based
// implementation to enable direct performance comparison
class browser_comparison_benchmark final : public benchmark {
  public:
    explicit browser_comparison_benchmark(memory::frame_allocator* allocator) noexcept
        : allocator_(allocator) {
    }

    [[nodiscard]] const char* name() const noexcept override {
        return "browser_comparison";
    }

    void setup() noexcept override {
        // Reset allocator to clean state
        if (allocator_) {
            allocator_->reset();
        }
        operation_results_ = 0;
    }

    void execute() noexcept override {
        // Simulate frame allocations (matching browser implementation)
        perform_allocations();

        // Simulate operations (matching browser implementation)
        perform_operations();

        // Reset for next frame
        if (allocator_) {
            allocator_->reset();
        }
    }

    void teardown() noexcept override {
        // Nothing to clean up
    }

    [[nodiscard]] bool verify() const noexcept override {
        // Verify we performed operations
        return operation_results_ > 0;
    }

    [[nodiscard]] uint64_t get_workload_hash() const noexcept override {
        // Hash based on configuration
        return 0xBFC0'BFCC'0001'0001ULL; // Browser Frame Comparison v1.1
    }

  private:
    void perform_allocations() noexcept {
        if (!allocator_) {
            return;
        }

        // Perform allocations matching browser pattern
        for (size_t i = 0; i < browser_comparison_config::allocations_per_frame; ++i) {
            // Select size using simple pattern (deterministic)
            const size_t size_idx = i % browser_comparison_config::allocation_size_count;
            const size_t size = browser_comparison_config::allocation_sizes[size_idx];

            // Allocate (8-byte alignment like JavaScript ArrayBuffer)
            void* ptr = allocator_->allocate(size, 8);

            // Touch the memory to ensure it's actually allocated
            if (ptr) {
                auto* bytes = static_cast<uint8_t*>(ptr);
                bytes[0] = static_cast<uint8_t>(i);
                if (size > 1) {
                    bytes[size - 1] = static_cast<uint8_t>(i);
                }
            }
        }
    }

    void perform_operations() noexcept {
        // Simulate computational work matching browser pattern
        double result = 0.0;

        for (size_t i = 0; i < browser_comparison_config::operations_per_frame; ++i) {
            // Simple math operations to simulate work
            const double fi = static_cast<double>(i);
            result += fi * fi + fi;
        }

        // Store result to prevent optimization
        operation_results_ = static_cast<uint64_t>(result);
    }

    memory::frame_allocator* allocator_;
    uint64_t operation_results_{0};
};

} // namespace aegis::core::benchmark
