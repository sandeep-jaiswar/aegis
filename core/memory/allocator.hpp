#pragma once

#include <cstddef>
#include <cstdint>

namespace aegis::core::memory {

// Base allocator interface - all allocators must implement this
// This provides a common interface for tracking and measuring allocations
class allocator {
  public:
    allocator() noexcept = default;
    virtual ~allocator() noexcept = default;

    // Disable copy and move - allocators have unique lifetime semantics
    allocator(const allocator&) = delete;
    allocator& operator=(const allocator&) = delete;
    allocator(allocator&&) = delete;
    allocator& operator=(allocator&&) = delete;

    // Allocate memory with specified size and alignment
    // Returns nullptr on failure (no exceptions in core)
    [[nodiscard]] virtual void* allocate(size_t size, size_t alignment) noexcept = 0;

    // Deallocate previously allocated memory
    // ptr must be a valid pointer returned by allocate() from this allocator
    virtual void deallocate(void* ptr, size_t size) noexcept = 0;

    // Reset allocator to initial state (if supported)
    // For arena/frame allocators, this invalidates all previous allocations
    virtual void reset() noexcept = 0;

    // Get allocation statistics
    [[nodiscard]] virtual size_t bytes_allocated() const noexcept = 0;
    [[nodiscard]] virtual size_t bytes_freed() const noexcept = 0;
    [[nodiscard]] virtual size_t bytes_in_use() const noexcept {
        return bytes_allocated() - bytes_freed();
    }
    [[nodiscard]] virtual size_t peak_bytes_used() const noexcept = 0;

    // Check if allocator can be reset
    [[nodiscard]] virtual bool supports_reset() const noexcept = 0;
};

// Helper to align a pointer/size to specified alignment
// Note: alignment must be a power of 2
[[nodiscard]] inline size_t align_up(size_t value, size_t alignment) noexcept {
    // Overflow check: ensure value + alignment - 1 doesn't overflow
    if (alignment == 0 || value > (static_cast<size_t>(-1) - alignment + 1)) {
        return value;
    }
    return (value + alignment - 1) & ~(alignment - 1);
}

[[nodiscard]] inline uintptr_t align_ptr_value(uintptr_t ptr, size_t alignment) noexcept {
    return align_up(ptr, alignment);
}

} // namespace aegis::core::memory
