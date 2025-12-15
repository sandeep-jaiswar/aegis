#include "core/memory/arena_allocator.hpp"

namespace aegis::core::memory {

void* arena_allocator::allocate(size_t size, size_t alignment) noexcept {
    if (size == 0) {
        return nullptr;
    }

    // Calculate aligned offset
    const size_t aligned_offset = align_up(offset, alignment);

    // Check if we have enough space
    if (aligned_offset + size > capacity) {
        return nullptr; // Out of memory
    }

    // Allocate from arena
    void* ptr = buffer + aligned_offset;
    offset = aligned_offset + size;

    // Update statistics
    total_allocated += size;
    if (offset > peak_used) {
        peak_used = offset;
    }

    return ptr;
}

} // namespace aegis::core::memory
