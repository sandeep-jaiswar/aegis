#pragma once

#include "core/layout/constraints.hpp"

#include <cstddef>
#include <cstdint>

namespace aegis::core::layout {

// Unique identifier for layout boxes
using box_id = uint32_t;

// Invalid box ID sentinel
constexpr box_id invalid_box_id = 0;

// Layout box - represents a single element in the layout tree
// This is POD-friendly and GPU-compatible
struct layout_box {
    // Identity
    box_id id{invalid_box_id};
    box_id parent_id{invalid_box_id};
    box_id first_child_id{invalid_box_id};
    box_id next_sibling_id{invalid_box_id};

    // Layout properties
    box_constraints constraints;
    padding pad;
    direction layout_direction{direction::vertical};
    alignment horizontal_align{alignment::start};
    alignment vertical_align{alignment::start};

    // Content size (for content-sized boxes)
    size content_size;

    // Computed layout (filled by layout engine)
    rect computed_rect;
    bool layout_computed{false};

    constexpr layout_box() noexcept = default;

    constexpr explicit layout_box(box_id id_val) noexcept 
        : id(id_val), parent_id(invalid_box_id), first_child_id(invalid_box_id), 
          next_sibling_id(invalid_box_id) {
    }
};

// Layout result - output of the layout engine
struct layout_result {
    // Number of boxes processed
    size_t box_count{0};

    // Whether layout succeeded
    bool success{true};

    // Error information (if success = false)
    box_id error_box_id{invalid_box_id};
};

} // namespace aegis::core::layout
