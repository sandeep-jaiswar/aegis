#include "core/layout/layout_engine.hpp"

namespace aegis::core::layout {

layout_box* layout_engine::find_box(layout_box* boxes, size_t box_count, box_id id) noexcept {
    if (id == invalid_box_id) {
        return nullptr;
    }

    // Linear search - O(n) but called once per box, so overall still O(n)
    for (size_t i = 0; i < box_count; ++i) {
        if (boxes[i].id == id) {
            return &boxes[i];
        }
    }
    return nullptr;
}

size layout_engine::compute_box_size(const layout_box& box, size available) noexcept {
    size result{};

    // Apply padding to available space
    const dimension available_width = available.width - box.pad.horizontal();
    const dimension available_height = available.height - box.pad.vertical();

    // Compute width
    switch (box.constraints.width_constraint) {
        case size_constraint::fixed:
            result.width = box.constraints.constrain_width(box.content_size.width);
            break;
        case size_constraint::min:
            result.width = box.constraints.constrain_width(box.content_size.width);
            if (result.width < available_width) {
                result.width = available_width;
            }
            break;
        case size_constraint::max:
            result.width = box.constraints.constrain_width(box.content_size.width);
            if (result.width > available_width) {
                result.width = available_width;
            }
            break;
        case size_constraint::fill:
            result.width = box.constraints.constrain_width(available_width);
            break;
        case size_constraint::content:
            result.width = box.constraints.constrain_width(box.content_size.width);
            break;
    }

    // Compute height
    switch (box.constraints.height_constraint) {
        case size_constraint::fixed:
            result.height = box.constraints.constrain_height(box.content_size.height);
            break;
        case size_constraint::min:
            result.height = box.constraints.constrain_height(box.content_size.height);
            if (result.height < available_height) {
                result.height = available_height;
            }
            break;
        case size_constraint::max:
            result.height = box.constraints.constrain_height(box.content_size.height);
            if (result.height > available_height) {
                result.height = available_height;
            }
            break;
        case size_constraint::fill:
            result.height = box.constraints.constrain_height(available_height);
            break;
        case size_constraint::content:
            result.height = box.constraints.constrain_height(box.content_size.height);
            break;
    }

    return result;
}

size layout_engine::compute_children_size(const layout_box* boxes, size_t box_count,
                                         const layout_box& parent) noexcept {
    size total{};

    box_id child_id = parent.first_child_id;
    while (child_id != invalid_box_id) {
        const layout_box* child = find_box(const_cast<layout_box*>(boxes), box_count, child_id);  // NOLINT(cppcoreguidelines-pro-type-const-cast)
        if (child == nullptr || !child->layout_computed) {
            break;
        }

        // Add child size based on parent's layout direction
        if (parent.layout_direction == direction::horizontal) {
            total.width = total.width + child->computed_rect.sz.width;
            if (child->computed_rect.sz.height > total.height) {
                total.height = child->computed_rect.sz.height;
            }
        } else {
            total.height = total.height + child->computed_rect.sz.height;
            if (child->computed_rect.sz.width > total.width) {
                total.width = child->computed_rect.sz.width;
            }
        }

        child_id = child->next_sibling_id;
    }

    return total;
}

void layout_engine::compute_sizes(layout_box* boxes, size_t box_count, box_id current_id,
                                 size available) noexcept {  // NOLINT(misc-no-recursion)
    layout_box* box = find_box(boxes, box_count, current_id);
    if (box == nullptr) {
        return;
    }

    // First, recursively compute sizes for all children
    box_id child_id = box->first_child_id;
    while (child_id != invalid_box_id) {
        // Pass available space minus padding to children
        const size child_available{
            available.width - box->pad.horizontal(),
            available.height - box->pad.vertical()
        };
        compute_sizes(boxes, box_count, child_id, child_available);

        const layout_box* child = find_box(boxes, box_count, child_id);
        if (child != nullptr) {
            child_id = child->next_sibling_id;
        } else {
            break;
        }
    }

    // Now compute this box's size based on its constraints and children
    if (box->constraints.width_constraint == size_constraint::content ||
        box->constraints.height_constraint == size_constraint::content) {
        // Update content_size based on children
        const size children_sz = compute_children_size(boxes, box_count, *box);
        
        if (box->constraints.width_constraint == size_constraint::content) {
            box->content_size.width = children_sz.width + box->pad.horizontal();
        }
        
        if (box->constraints.height_constraint == size_constraint::content) {
            box->content_size.height = children_sz.height + box->pad.vertical();
        }
    }

    // Compute final size
    const size computed = compute_box_size(*box, available);
    box->computed_rect.sz = computed;
    box->layout_computed = true;
}

void layout_engine::compute_positions(layout_box* boxes, size_t box_count, box_id current_id,
                                     position parent_pos) noexcept {  // NOLINT(misc-no-recursion)
    layout_box* box = find_box(boxes, box_count, current_id);
    if (box == nullptr) {
        return;
    }

    // Set this box's position
    box->computed_rect.pos = parent_pos;

    // Compute positions for children
    position child_pos = parent_pos;
    child_pos.x = child_pos.x + box->pad.left;
    child_pos.y = child_pos.y + box->pad.top;

    box_id child_id = box->first_child_id;
    while (child_id != invalid_box_id) {
        layout_box* child = find_box(boxes, box_count, child_id);
        if (child == nullptr) {
            break;
        }

        // Apply alignment
        position aligned_pos = child_pos;

        // Horizontal alignment
        const dimension available_width = box->computed_rect.sz.width - box->pad.horizontal();
        if (box->horizontal_align == alignment::center) {
            const dimension offset = (available_width - child->computed_rect.sz.width);
            aligned_pos.x = aligned_pos.x + dimension(offset.value * 0.5F);
        } else if (box->horizontal_align == alignment::end) {
            aligned_pos.x = aligned_pos.x + (available_width - child->computed_rect.sz.width);
        }

        // Vertical alignment
        const dimension available_height = box->computed_rect.sz.height - box->pad.vertical();
        if (box->vertical_align == alignment::center) {
            const dimension offset = (available_height - child->computed_rect.sz.height);
            aligned_pos.y = aligned_pos.y + dimension(offset.value * 0.5F);
        } else if (box->vertical_align == alignment::end) {
            aligned_pos.y = aligned_pos.y + (available_height - child->computed_rect.sz.height);
        }

        // Recursively position this child and its descendants
        compute_positions(boxes, box_count, child_id, aligned_pos);

        // Move position for next sibling based on layout direction
        if (box->layout_direction == direction::horizontal) {
            child_pos.x = child_pos.x + child->computed_rect.sz.width;
        } else {
            child_pos.y = child_pos.y + child->computed_rect.sz.height;
        }

        child_id = child->next_sibling_id;
    }
}

layout_result layout_engine::compute_layout(layout_box* boxes, size_t box_count,
                                           box_id root_id, size available_space) noexcept {
    layout_result result{};

    if (boxes == nullptr || box_count == 0) {
        result.success = false;
        return result;
    }

    // Reset all boxes
    for (size_t i = 0; i < box_count; ++i) {
        boxes[i].layout_computed = false;
    }

    // One-pass layout: two phases
    // Phase 1: Compute sizes bottom-up (children before parents)
    compute_sizes(boxes, box_count, root_id, available_space);

    // Phase 2: Compute positions top-down (parents before children)
    compute_positions(boxes, box_count, root_id, position{dimension(0.0F), dimension(0.0F)});

    result.box_count = box_count;
    result.success = true;

    return result;
}

} // namespace aegis::core::layout
