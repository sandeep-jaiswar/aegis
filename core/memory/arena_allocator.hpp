#pragma once

#include "core/memory/allocator.hpp"

namespace aegis::core::memory {

// Arena allocator - linear allocator with bulk reset
// Efficient for allocating many objects that are freed all at once
// Does not support individual deallocation - only reset()
//
// Memory layout:
// [--------------- buffer (capacity) ---------------]
// [--- used ---][-------- available ---------------]
//              ^
//           offset
class arena_allocator final : public allocator {
  public:
    // Create arena with pre-allocated buffer
    // buffer must remain valid for the lifetime of the allocator
    arena_allocator(void* buffer, size_t capacity) noexcept
        : buffer(static_cast<uint8_t*>(buffer))
        , capacity(capacity)
        , offset(0)
        , total_allocated(0)
        , total_freed(0)
        , peak_used(0) {
    }

    ~arena_allocator() noexcept override = default;

    // Disable copy and move
    arena_allocator(const arena_allocator&) = delete;
    arena_allocator& operator=(const arena_allocator&) = delete;
    arena_allocator(arena_allocator&&) = delete;
    arena_allocator& operator=(arena_allocator&&) = delete;

    // Allocate from the arena
    // Returns nullptr if insufficient space
    [[nodiscard]] void* allocate(size_t size, size_t alignment) noexcept override;

    // Arena does not support individual deallocation
    // All memory is freed together via reset()
    void deallocate(void* ptr, size_t size) noexcept override {
        // Track for statistics, but don't actually free
        (void)ptr;
        total_freed += size;
    }

    // Reset arena to initial state - invalidates all allocations
    void reset() noexcept override {
        offset = 0;
        total_allocated = 0;
        total_freed = 0;
        peak_used = 0;
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

    // Arena-specific queries
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
    size_t offset;
    size_t total_allocated;
    size_t total_freed;
    size_t peak_used;
};

} // namespace aegis::core::memory
