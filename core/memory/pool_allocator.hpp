#pragma once

#include "core/memory/allocator.hpp"

namespace aegis::core::memory {

// Pool allocator - fixed-size object allocator
// Efficient for allocating/deallocating many objects of the same size
// Uses a free list for O(1) allocation and deallocation
//
// Memory layout:
// [block0][block1][block2]...[blockN]
//    ^       ^       ^
//    |-------|-------|  (free list when not allocated)
//
// Each free block stores a pointer to the next free block
class pool_allocator final : public allocator {
  public:
    // Create pool allocator
    // buffer: pre-allocated memory for pool
    // capacity: total size of buffer in bytes
    // block_size: size of each allocation (must be >= sizeof(void*))
    // block_alignment: alignment of each block
    pool_allocator(void* buffer, size_t capacity, size_t block_size,
                   size_t block_alignment) noexcept;

    ~pool_allocator() noexcept override = default;

    // Disable copy and move
    pool_allocator(const pool_allocator&) = delete;
    pool_allocator& operator=(const pool_allocator&) = delete;
    pool_allocator(pool_allocator&&) = delete;
    pool_allocator& operator=(pool_allocator&&) = delete;

    // Allocate one block from pool
    // size and alignment are ignored - all blocks are fixed size
    // Returns nullptr if pool is exhausted
    [[nodiscard]] void* allocate(size_t size, size_t alignment) noexcept override;

    // Return block to pool
    // ptr must be a valid pointer returned by allocate() from this pool
    // size is ignored
    void deallocate(void* ptr, size_t size) noexcept override;

    // Reset pool - returns all blocks to free list
    void reset() noexcept override;

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

    // Pool-specific queries
    [[nodiscard]] size_t get_block_size() const noexcept {
        return block_size;
    }

    [[nodiscard]] size_t get_block_count() const noexcept {
        return block_count;
    }

    [[nodiscard]] size_t blocks_in_use() const noexcept {
        return blocks_used;
    }

    [[nodiscard]] size_t blocks_available() const noexcept {
        return block_count - blocks_used;
    }

  private:
    void initialize_free_list() noexcept;

    uint8_t* buffer;
    size_t capacity;
    size_t block_size;
    size_t block_alignment;
    size_t block_count{0};
    void* free_list_head{nullptr};
    size_t blocks_used{0};
    size_t total_allocated{0};
    size_t total_freed{0};
    size_t peak_used{0};
};

} // namespace aegis::core::memory
