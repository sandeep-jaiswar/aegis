#pragma once

#include "core/benchmark/benchmark.hpp"
#include "core/memory/frame_allocator.hpp"

namespace aegis::core::benchmark {

// Benchmark for frame allocator performance
class frame_allocator_benchmark final : public benchmark {
  public:
    explicit frame_allocator_benchmark(memory::frame_allocator* alloc) noexcept : allocator(alloc) {
    }

    [[nodiscard]] const char* name() const noexcept override {
        return "frame_allocator_benchmark";
    }

    void setup() noexcept override {
        if (allocator) {
            allocator->reset();
        }
    }

    void execute() noexcept override {
        if (!allocator) {
            return;
        }

        // Simulate typical frame allocations
        // Small allocations (common for temporary objects)
        void* ptr1 = allocator->allocate(32, 8);
        void* ptr2 = allocator->allocate(64, 8);
        void* ptr3 = allocator->allocate(128, 16);

        // Medium allocations (UI elements, transforms)
        void* ptr4 = allocator->allocate(256, 16);
        void* ptr5 = allocator->allocate(512, 32);

        // Large allocation (render data)
        void* ptr6 = allocator->allocate(4096, 64);

        // Touch allocated memory to prevent optimization
        if (ptr1)
            *static_cast<uint8_t*>(ptr1) = 1;
        if (ptr2)
            *static_cast<uint8_t*>(ptr2) = 2;
        if (ptr3)
            *static_cast<uint8_t*>(ptr3) = 3;
        if (ptr4)
            *static_cast<uint8_t*>(ptr4) = 4;
        if (ptr5)
            *static_cast<uint8_t*>(ptr5) = 5;
        if (ptr6)
            *static_cast<uint8_t*>(ptr6) = 6;

        // Deallocate (frame allocator tracks but doesn't free immediately)
        if (ptr1)
            allocator->deallocate(ptr1, 32);
        if (ptr2)
            allocator->deallocate(ptr2, 64);
        if (ptr3)
            allocator->deallocate(ptr3, 128);
        if (ptr4)
            allocator->deallocate(ptr4, 256);
        if (ptr5)
            allocator->deallocate(ptr5, 512);
        if (ptr6)
            allocator->deallocate(ptr6, 4096);
    }

    void teardown() noexcept override {
        if (allocator) {
            allocator->reset();
        }
    }

    [[nodiscard]] bool verify() const noexcept override {
        // Verify allocator is in consistent state
        return allocator != nullptr;
    }

    [[nodiscard]] uint64_t get_workload_hash() const noexcept override {
        // Hash based on allocation pattern
        return 0x123456789ABCDEF0ULL;
    }

  private:
    memory::frame_allocator* allocator;
};

} // namespace aegis::core::benchmark
