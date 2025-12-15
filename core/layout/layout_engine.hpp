#pragma once

#include "core/layout/layout_box.hpp"
#include "core/memory/allocator.hpp"

#include <cstddef>

namespace aegis::core::layout {

// One-pass layout engine
// Guarantees:
// - O(n) complexity where n is number of boxes
// - Deterministic: same inputs → same outputs
// - No cascading recalculations
// - Single pass through the layout tree
class layout_engine {
  public:
    layout_engine() noexcept = default;
    ~layout_engine() noexcept = default;

    // Disable copy and move
    layout_engine(const layout_engine&) = delete;
    layout_engine& operator=(const layout_engine&) = delete;
    layout_engine(layout_engine&&) = delete;
    layout_engine& operator=(layout_engine&&) = delete;

    // Compute layout for a tree of boxes
    // Performs a single pass: computes sizes bottom-up, positions top-down
    // boxes: array of layout boxes (order doesn't matter, boxes are found by ID)
    // box_count: number of boxes in the array
    // root_id: ID of the root box
    // available_space: available space for the root box
    [[nodiscard]] layout_result compute_layout(layout_box* boxes, size_t box_count, box_id root_id,
                                               size available_space) noexcept;

  private:
    // First pass: compute sizes bottom-up (children before parents)
    void compute_sizes(layout_box* boxes, size_t box_count, box_id current_id,
                       size available) noexcept;

    // Second pass: compute positions top-down (parents before children)
    void compute_positions(layout_box* boxes, size_t box_count, box_id current_id,
                           position parent_pos) noexcept;

    // Helper: find box by ID in linear array
    [[nodiscard]] static layout_box* find_box(layout_box* boxes, size_t box_count,
                                              box_id id) noexcept;
    [[nodiscard]] static const layout_box* find_box(const layout_box* boxes, size_t box_count,
                                                    box_id id) noexcept;

    // Helper: compute size for a single box based on its constraints
    [[nodiscard]] static size compute_box_size(const layout_box& box, size available) noexcept;

    // Helper: compute total size of children
    [[nodiscard]] static size compute_children_size(const layout_box* boxes, size_t box_count,
                                                    const layout_box& parent) noexcept;
};

} // namespace aegis::core::layout
