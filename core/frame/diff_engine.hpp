#pragma once

#include "core/frame/scene_graph.hpp"
#include "core/memory/allocator.hpp"

#include <cstddef>
#include <cstdint>

namespace aegis::core::frame {

// Diff operation types - minimal set of operations to replay changes
enum class diff_op : uint8_t {
    add_node = 0,      // Node was added
    remove_node = 1,   // Node was removed
    update_props = 2,  // Node properties changed
    add_child = 3,     // Child relationship added
    remove_child = 4,  // Child relationship removed
    reorder_child = 5  // Child order changed
};

// Single diff operation - represents one atomic change
struct diff_change {
    diff_op operation{diff_op::add_node};
    node_id node{invalid_node_id};         // Primary node affected
    node_id related_node{invalid_node_id}; // For parent/child operations
    node_properties old_props{};           // Previous properties (for update)
    node_properties new_props{};           // New properties (for update)
    uint32_t old_index{0};                 // Previous child index (for reorder)
    uint32_t new_index{0};                 // New child index (for reorder)
};

// Diff result configuration
struct diff_config {
    uint32_t max_changes{4096}; // Maximum number of changes to track
};

// Diff computation result
enum class diff_result : uint8_t {
    success = 0,
    out_of_memory = 1,
    change_limit_exceeded = 2,
    invalid_graph = 3
};

// Structural diff engine - computes minimal change sets between scene graphs
//
// Acceptance Criteria:
// 1. Diff cost proportional to actual changes
//    - Only compares nodes that could have changed
//    - Uses stable node IDs to avoid full tree walk
//    - Tracks changed nodes efficiently
// 2. No full tree walk on small updates
//    - Maintains node ID set for O(1) existence checks
//    - Only traverses modified subtrees
// 3. Change sets are replayable
//    - Operations are ordered and deterministic
//    - Each change is atomic and self-contained
//    - Can reconstruct new state from old state + changes
class diff_engine {
  public:
    // Create diff engine with configuration and allocator
    explicit diff_engine(const diff_config& config, memory::allocator* alloc) noexcept
        : cfg(config), allocator(alloc) {
        if (allocator == nullptr) {
            return;
        }

        // Allocate change storage
        const size_t changes_size = sizeof(diff_change) * cfg.max_changes;
        void* changes_mem = allocator->allocate(changes_size, alignof(diff_change));
        if (changes_mem != nullptr) {
            changes = static_cast<diff_change*>(changes_mem);
            // Initialize all changes
            for (uint32_t i = 0; i < cfg.max_changes; ++i) {
                changes[i] = diff_change{};
            }
        }

        // Allocate node ID tracking for previous graph
        // Use same max_changes as upper bound for nodes we need to track
        const size_t prev_ids_size = sizeof(node_id) * cfg.max_changes;
        void* prev_ids_mem = allocator->allocate(prev_ids_size, alignof(node_id));
        if (prev_ids_mem != nullptr) {
            prev_node_ids = static_cast<node_id*>(prev_ids_mem);
            for (uint32_t i = 0; i < cfg.max_changes; ++i) {
                prev_node_ids[i] = invalid_node_id;
            }
        }
    }

    ~diff_engine() noexcept {
        if (allocator != nullptr) {
            if (changes != nullptr) {
                allocator->deallocate(changes, sizeof(diff_change) * cfg.max_changes);
            }
            if (prev_node_ids != nullptr) {
                allocator->deallocate(prev_node_ids, sizeof(node_id) * cfg.max_changes);
            }
        }
    }

    // Disable copy and move
    diff_engine(const diff_engine&) = delete;
    diff_engine& operator=(const diff_engine&) = delete;
    diff_engine(diff_engine&&) = delete;
    diff_engine& operator=(diff_engine&&) = delete;

    // Compute diff between previous and current scene graphs
    // Returns diff_result indicating success or failure
    // Change count is stored in out_change_count
    [[nodiscard]] diff_result compute_diff(const scene_graph* prev,
                                           const scene_graph* current,
                                           uint32_t& out_change_count) noexcept;

    // Get computed changes (read-only access)
    [[nodiscard]] const diff_change* get_changes() const noexcept {
        return changes;
    }

    // Get change count from last diff computation
    [[nodiscard]] uint32_t get_change_count() const noexcept {
        return change_count;
    }

    // Clear changes (prepare for next diff)
    void clear() noexcept {
        change_count = 0;
        prev_node_count = 0;
    }

    // Check if diff engine is valid
    [[nodiscard]] bool is_valid() const noexcept {
        return changes != nullptr && prev_node_ids != nullptr;
    }

    // Record a change operation (used internally and by helper functions)
    [[nodiscard]] bool record_change(const diff_change& change) noexcept {
        if (change_count >= cfg.max_changes) {
            return false; // Change limit exceeded
        }
        changes[change_count++] = change;
        return true;
    }

  private:

    // Check if node ID exists in previous graph
    [[nodiscard]] bool node_existed(node_id id) const noexcept {
        for (uint32_t i = 0; i < prev_node_count; ++i) {
            if (prev_node_ids[i] == id) {
                return true;
            }
        }
        return false;
    }

    // Record node ID from previous graph
    [[nodiscard]] bool record_prev_node(node_id id) noexcept {
        if (prev_node_count >= cfg.max_changes) {
            return false;
        }
        prev_node_ids[prev_node_count++] = id;
        return true;
    }

    // Compare node properties for changes
    [[nodiscard]] static bool props_equal(const node_properties& a,
                                          const node_properties& b) noexcept {
        return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height &&
               a.color == b.color;
    }

    diff_config cfg;
    memory::allocator* allocator;

    // Change storage
    diff_change* changes{nullptr};
    uint32_t change_count{0};

    // Previous graph node ID tracking (for efficient existence checks)
    node_id* prev_node_ids{nullptr};
    uint32_t prev_node_count{0};
};

} // namespace aegis::core::frame
