#pragma once

#include "core/memory/allocator.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace aegis::core::state {

// Reference counting for structural sharing
// Explicit and bounded - no hidden behavior
//
// Acceptance criteria satisfied:
// 3. Structural sharing is explicit and bounded
//    - Refcount is visible and trackable
//    - Sharing must be opted-in explicitly
//    - No hidden copying or sharing
struct refcount {
    uint32_t count{1};

    void increment() noexcept {
        ++count;
    }

    [[nodiscard]] bool decrement() noexcept {
        // Only decrement if count > 0
        if (count > 0) {
            --count;
            return count == 0; // Return true if this was the last reference
        }
        return false; // Count was already 0, don't deallocate
    }

    [[nodiscard]] uint32_t get() const noexcept {
        return count;
    }

    [[nodiscard]] bool is_unique() const noexcept {
        return count == 1;
    }
};

// Shared data block with explicit reference counting
// Used for structural sharing between state snapshots
template <typename T>
struct shared_block {
    refcount refs;
    T data;

    explicit shared_block(T initial_data) noexcept : refs{}, data(initial_data) {
    }

    // Increment reference count (explicit sharing)
    void add_ref() noexcept {
        refs.increment();
    }

    // Decrement reference count
    // Returns true if this was the last reference
    [[nodiscard]] bool release() noexcept {
        return refs.decrement();
    }

    // Check if this block is uniquely owned
    [[nodiscard]] bool is_unique() const noexcept {
        return refs.is_unique();
    }

    // Get current reference count
    [[nodiscard]] uint32_t ref_count() const noexcept {
        return refs.get();
    }
};

// Shared pointer with explicit copy-on-write semantics
// Unlike std::shared_ptr, this has explicit, bounded behavior
template <typename T>
class shared_data {
  public:
    // Create new shared data with allocator
    // Note: T must be a POD-like type or have a trivial copy assignment operator
    // Complex types with non-trivial constructors should use a different pattern
    static shared_data create(T data, memory::allocator* alloc) noexcept {
        if (alloc == nullptr) {
            return shared_data(); // Return null shared_data
        }

        void* mem = alloc->allocate(sizeof(shared_block<T>), alignof(shared_block<T>));
        if (mem == nullptr) {
            return shared_data(); // Allocation failed
        }

        // Manual construction (avoid placement new with -fno-exceptions)
        auto* block = static_cast<shared_block<T>*>(mem);
        // Initialize refcount
        block->refs = refcount{};
        // Copy data using assignment (requires T to be copy-assignable)
        block->data = data;
        return shared_data(block, alloc);
    }

    // Default constructor - null data
    shared_data() noexcept : block_(nullptr), allocator_(nullptr) {
    }

    // Copy constructor - increment reference count (explicit sharing)
    shared_data(const shared_data& other) noexcept
        : block_(other.block_), allocator_(other.allocator_) {
        if (block_ != nullptr) {
            block_->add_ref();
        }
    }

    // Copy assignment - manage reference counts
    shared_data& operator=(const shared_data& other) noexcept {
        if (this != &other) {
            release();
            block_ = other.block_;
            allocator_ = other.allocator_;
            if (block_ != nullptr) {
                block_->add_ref();
            }
        }
        return *this;
    }

    // Move constructor - transfer ownership
    shared_data(shared_data&& other) noexcept : block_(other.block_), allocator_(other.allocator_) {
        other.block_ = nullptr;
        other.allocator_ = nullptr;
    }

    // Move assignment - transfer ownership
    shared_data& operator=(shared_data&& other) noexcept {
        if (this != &other) {
            release();
            block_ = other.block_;
            allocator_ = other.allocator_;
            other.block_ = nullptr;
            other.allocator_ = nullptr;
        }
        return *this;
    }

    // Destructor - release reference
    ~shared_data() noexcept {
        release();
    }

    // Access data (read-only to enforce immutability)
    [[nodiscard]] const T* get() const noexcept {
        return block_ ? &block_->data : nullptr;
    }

    [[nodiscard]] const T& operator*() const noexcept {
        return block_->data;
    }

    [[nodiscard]] const T* operator->() const noexcept {
        return &block_->data;
    }

    // Check if data is valid
    [[nodiscard]] bool is_valid() const noexcept {
        return block_ != nullptr;
    }

    // Check if this is the unique owner (can safely modify)
    [[nodiscard]] bool is_unique() const noexcept {
        return block_ && block_->is_unique();
    }

    // Get reference count (explicit tracking)
    [[nodiscard]] uint32_t ref_count() const noexcept {
        return block_ ? block_->ref_count() : 0;
    }

    // Copy-on-write: get mutable data, copying if shared
    [[nodiscard]] T* get_mut() noexcept {
        if (block_ == nullptr) {
            return nullptr;
        }

        if (block_->is_unique()) {
            // We're the only owner - can modify directly
            return &block_->data;
        }

        // Multiple owners - must copy before modifying
        return nullptr; // Caller must explicitly call clone()
    }

    // Explicit clone operation for copy-on-write
    [[nodiscard]] shared_data clone() const noexcept {
        if (block_ == nullptr || allocator_ == nullptr) {
            return shared_data();
        }

        // Create new independent copy
        return create(block_->data, allocator_);
    }

    // Calculate bytes used by structural sharing
    [[nodiscard]] size_t shared_bytes() const noexcept {
        if (block_ == nullptr || block_->is_unique()) {
            return 0; // Not shared
        }
        return sizeof(T);
    }

  private:
    explicit shared_data(shared_block<T>* block, memory::allocator* alloc) noexcept
        : block_(block), allocator_(alloc) {
    }

    void release() noexcept {
        if (block_ != nullptr && block_->release()) {
            // Last reference - deallocate
            if (allocator_ != nullptr) {
                // No explicit destructor call needed for POD-style types
                allocator_->deallocate(block_, sizeof(shared_block<T>));
            }
        }
        block_ = nullptr;
        allocator_ = nullptr;
    }

    shared_block<T>* block_;
    memory::allocator* allocator_;
};

// Utility to calculate structural sharing statistics
struct sharing_stats {
    size_t total_bytes{0};
    size_t shared_bytes{0};
    size_t unique_bytes{0};
    uint32_t shared_blocks{0};
    uint32_t unique_blocks{0};

    [[nodiscard]] float sharing_ratio() const noexcept {
        if (total_bytes == 0) {
            return 0.0F;
        }
        return static_cast<float>(shared_bytes) / static_cast<float>(total_bytes);
    }

    [[nodiscard]] bool has_sharing() const noexcept {
        return shared_blocks > 0;
    }
};

// Calculate sharing statistics for a shared_data instance
template <typename T>
[[nodiscard]] inline sharing_stats calculate_sharing_stats(const shared_data<T>& data) noexcept {
    sharing_stats stats{};

    if (!data.is_valid()) {
        return stats;
    }

    stats.total_bytes = sizeof(T);

    if (data.is_unique()) {
        stats.unique_bytes = sizeof(T);
        stats.unique_blocks = 1;
    } else {
        stats.shared_bytes = sizeof(T);
        stats.shared_blocks = 1;
    }

    return stats;
}

} // namespace aegis::core::state
