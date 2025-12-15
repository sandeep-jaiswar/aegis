#include "core/memory/pool_allocator.hpp"

namespace aegis::core::memory {

pool_allocator::pool_allocator(void* buffer_ptr, size_t capacity_val, size_t block_size_val,
                               size_t block_alignment_val) noexcept
    : buffer(static_cast<uint8_t*>(buffer_ptr)), capacity(capacity_val), block_size(block_size_val),
      block_alignment(block_alignment_val) {
    // Block size must be at least large enough to store a pointer
    if (block_size < sizeof(void*)) {
        block_size = sizeof(void*);
    }

    // Align block size to block alignment
    block_size = align_up(block_size, block_alignment);

    // Calculate how many blocks fit in the buffer
    block_count = capacity / block_size;

    // Initialize free list
    initialize_free_list();
}

void pool_allocator::initialize_free_list() noexcept {
    if (block_count == 0) {
        free_list_head = nullptr;
        return;
    }

    // Build free list - each block points to the next
    free_list_head = buffer;

    for (size_t i = 0; i < block_count - 1; ++i) {
        void* current = buffer + (i * block_size);
        void* next = buffer + ((i + 1) * block_size);
        *static_cast<void**>(current) = next;
    }

    // Last block points to null
    void* last = buffer + ((block_count - 1) * block_size);
    *static_cast<void**>(last) = nullptr;
}

void* pool_allocator::allocate(size_t size, size_t alignment) noexcept {
    // Ignore size and alignment - all allocations are fixed size
    (void)size;
    (void)alignment;

    if (free_list_head == nullptr) {
        return nullptr; // Pool exhausted
    }

    // Pop from free list
    void* ptr = free_list_head;
    free_list_head = *static_cast<void**>(free_list_head);

    // Update statistics
    blocks_used++;
    total_allocated += block_size;

    const size_t bytes_used = blocks_used * block_size;
    if (bytes_used > peak_used) {
        peak_used = bytes_used;
    }

    return ptr;
}

void pool_allocator::deallocate(void* ptr, size_t size) noexcept {
    if (ptr == nullptr) {
        return;
    }

    (void)size; // Ignore size parameter

    // Push onto free list
    *static_cast<void**>(ptr) = free_list_head;
    free_list_head = ptr;

    // Update statistics
    blocks_used--;
    total_freed += block_size;
}

void pool_allocator::reset() noexcept {
    // Rebuild free list
    initialize_free_list();

    // Reset statistics
    blocks_used = 0;
    total_allocated = 0;
    total_freed = 0;
    peak_used = 0;
}

} // namespace aegis::core::memory
