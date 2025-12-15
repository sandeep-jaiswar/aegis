#include "core/frame/diff_engine.hpp"

namespace aegis::core::frame {

// Helper: Check for child relationship changes
static diff_result check_child_changes(diff_engine* engine,
                                       const scene_graph* prev,
                                       const scene_graph* current,
                                       node_id node_id_val,
                                       uint32_t& out_change_count) noexcept {
    uint32_t prev_child_count = 0;
    uint32_t curr_child_count = 0;
    const node_id* prev_children = prev->get_children(node_id_val, prev_child_count);
    const node_id* curr_children = current->get_children(node_id_val, curr_child_count);

    // Detect removed children
    if (prev_children != nullptr) {
        for (uint32_t j = 0; j < prev_child_count; ++j) {
            const node_id child_id = prev_children[j];
            bool found = false;

            if (curr_children != nullptr) {
                for (uint32_t k = 0; k < curr_child_count; ++k) {
                    if (curr_children[k] == child_id) {
                        found = true;
                        break;
                    }
                }
            }

            if (!found) {
                const diff_change change{.operation = diff_op::remove_child,
                                        .node = node_id_val,
                                        .related_node = child_id,
                                        .old_props = {},
                                        .new_props = {},
                                        .old_index = j,
                                        .new_index = 0};
                if (!engine->record_change(change)) {
                    out_change_count = engine->get_change_count();
                    return diff_result::change_limit_exceeded;
                }
            }
        }
    }

    // Detect added children and reordering
    if (curr_children != nullptr) {
        for (uint32_t j = 0; j < curr_child_count; ++j) {
            const node_id child_id = curr_children[j];
            bool found = false;
            uint32_t old_index = 0;

            if (prev_children != nullptr) {
                for (uint32_t k = 0; k < prev_child_count; ++k) {
                    if (prev_children[k] == child_id) {
                        found = true;
                        old_index = k;
                        break;
                    }
                }
            }

            if (!found) {
                const diff_change change{.operation = diff_op::add_child,
                                        .node = node_id_val,
                                        .related_node = child_id,
                                        .old_props = {},
                                        .new_props = {},
                                        .old_index = 0,
                                        .new_index = j};
                if (!engine->record_change(change)) {
                    out_change_count = engine->get_change_count();
                    return diff_result::change_limit_exceeded;
                }
            } else if (old_index != j) {
                const diff_change change{.operation = diff_op::reorder_child,
                                        .node = node_id_val,
                                        .related_node = child_id,
                                        .old_props = {},
                                        .new_props = {},
                                        .old_index = old_index,
                                        .new_index = j};
                if (!engine->record_change(change)) {
                    out_change_count = engine->get_change_count();
                    return diff_result::change_limit_exceeded;
                }
            }
        }
    }

    return diff_result::success;
}

// Compute structural diff between two scene graphs
// This algorithm is designed to:
// 1. Have cost proportional to actual changes (not full tree size)
// 2. Avoid full tree walk when changes are small
// 3. Produce replayable change sets
diff_result diff_engine::compute_diff(const scene_graph* prev,
                                     const scene_graph* current,
                                     uint32_t& out_change_count) noexcept {
    // Reset state
    clear();

    // Validate inputs
    if (prev == nullptr || current == nullptr) {
        out_change_count = 0;
        return diff_result::invalid_graph;
    }

    if (!prev->is_valid() || !current->is_valid()) {
        out_change_count = 0;
        return diff_result::invalid_graph;
    }

    // Build previous graph node ID set for O(1) existence checks
    const uint32_t prev_count = prev->count();
    for (uint32_t i = 0; i < prev_count; ++i) {
        const scene_node* prev_node = prev->get_node(root_node_id + i);
        if (prev_node != nullptr && prev_node->is_valid()) {
            if (!record_prev_node(prev_node->id)) {
                out_change_count = change_count;
                return diff_result::out_of_memory;
            }
        }
    }

    // Phase 1: Detect removed and updated nodes
    for (uint32_t i = 0; i < prev_node_count; ++i) {
        const node_id prev_id = prev_node_ids[i];
        const scene_node* prev_node = prev->get_node(prev_id);
        if (prev_node == nullptr) {
            continue;
        }

        const scene_node* curr_node = current->get_node(prev_id);
        if (curr_node == nullptr) {
            // Node was removed
            const diff_change change{.operation = diff_op::remove_node,
                                    .node = prev_id,
                                    .related_node = prev_node->parent_id,
                                    .old_props = prev_node->props,
                                    .new_props = {},
                                    .old_index = 0,
                                    .new_index = 0};
            if (!record_change(change)) {
                out_change_count = change_count;
                return diff_result::change_limit_exceeded;
            }
        } else {
            // Check for property changes
            if (!props_equal(prev_node->props, curr_node->props)) {
                const diff_change change{.operation = diff_op::update_props,
                                        .node = prev_id,
                                        .related_node = invalid_node_id,
                                        .old_props = prev_node->props,
                                        .new_props = curr_node->props,
                                        .old_index = 0,
                                        .new_index = 0};
                if (!record_change(change)) {
                    out_change_count = change_count;
                    return diff_result::change_limit_exceeded;
                }
            }

            // Check for child relationship changes
            const diff_result child_result =
                check_child_changes(this, prev, current, prev_id, out_change_count);
            if (child_result != diff_result::success) {
                return child_result;
            }
        }
    }

    // Phase 2: Detect added nodes
    const uint32_t curr_count = current->count();
    for (node_id check_id = root_node_id; check_id < root_node_id + curr_count; ++check_id) {
        const scene_node* node = current->get_node(check_id);
        if (node != nullptr && node->is_valid()) {
            if (!node_existed(check_id)) {
                const diff_change change{.operation = diff_op::add_node,
                                        .node = check_id,
                                        .related_node = node->parent_id,
                                        .old_props = {},
                                        .new_props = node->props,
                                        .old_index = 0,
                                        .new_index = 0};
                if (!record_change(change)) {
                    out_change_count = change_count;
                    return diff_result::change_limit_exceeded;
                }
            }
        }
    }

    out_change_count = change_count;
    return diff_result::success;
}

} // namespace aegis::core::frame
