#pragma once

#include "core/memory/allocator.hpp"

namespace aegis::core::memory {

// Frame allocator - per-frame linear allocator
// Optimized for allocations that live for exactly one frame
// Automatically resets at the end of each frame
//
// Key properties:
// - O(1) allocation
// - O(1) reset (critical for frame boundaries)
// - No individual deallocation tracking needed
// - Predictable memory usage per frame
class frame_allocator final : public allocator {
  public:
    // Create frame allocator with pre-allocated buffer
    // buffer must remain valid for the lifetime of the allocator
    explicit frame_allocator(void* buffer_ptr, size_t capacity_val) noexcept
        : buffer(static_cast<uint8_t*>(buffer_ptr)), capacity(capacity_val) {
    }

    ~frame_allocator() noexcept override = default;

    // Disable copy and move
    frame_allocator(const frame_allocator&) = delete;
    frame_allocator& operator=(const frame_allocator&) = delete;
    frame_allocator(frame_allocator&&) = delete;
    frame_allocator& operator=(frame_allocator&&) = delete;

    // Allocate memory for this frame
    // Returns nullptr if insufficient space
    [[nodiscard]] void* allocate(size_t size, size_t alignment) noexcept override;

    // Frame allocator does not support individual deallocation
    // Memory is freed at frame boundary via reset()
    void deallocate(void* ptr, size_t size) noexcept override {
        // Track for statistics only
        (void)ptr;
        total_freed += size;
        deallocation_count++;
    }

    // Reset frame allocator - called at end_frame()
    // This is O(1) and invalidates all allocations from this frame
    void reset() noexcept override {
        offset = 0;
        total_allocated = 0;
        total_freed = 0;
        peak_used = 0;
        allocation_count = 0;
        deallocation_count = 0;
    }

    // Statistics
    [[nodiscard]] size_t bytes_allocated() const noexcept override {
        return total_allocated;
    }

    [[nodiscard]] size_t bytes_freed() const noexcept override {
        return total_freed;
    }

    [[nodiscard]] size_t peak_bytes_used() const noexcept override {
        return peak_used;
    }

    [[nodiscard]] bool supports_reset() const noexcept override {
        return true;
    }

    // Frame-specific metrics
    [[nodiscard]] size_t get_allocation_count() const noexcept {
        return allocation_count;
    }

    [[nodiscard]] size_t get_deallocation_count() const noexcept {
        return deallocation_count;
    }

    [[nodiscard]] size_t get_capacity() const noexcept {
        return capacity;
    }

    [[nodiscard]] size_t bytes_used() const noexcept {
        return offset;
    }

    [[nodiscard]] size_t bytes_available() const noexcept {
        return capacity - offset;
    }

  private:
    uint8_t* buffer;
    size_t capacity;
    size_t offset{0};
    size_t total_allocated{0};
    size_t total_freed{0};
    size_t peak_used{0};
    size_t allocation_count{0};
    size_t deallocation_count{0};
};

} // namespace aegis::core::memory
