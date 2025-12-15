#include "core/frame/gpu_command_buffer.hpp"

namespace aegis::core::frame {

// Helper: Get buffer ID for node
// In a real implementation, this would map node properties to GPU buffers
// For now, we use a simple hash of node ID
static uint32_t get_buffer_id_for_node(node_id id) noexcept {
    // Simple hash: node_id mod 256 gives us buffer ID
    // In production, this would be a proper buffer allocation system
    return static_cast<uint32_t>(id % 256);
}

// Helper: Get pipeline ID for node type
// Maps node types to different rendering pipelines
static uint32_t get_pipeline_id_for_node(node_type type) noexcept {
    switch (type) {
    case node_type::rectangle:
        return 1; // Rectangle shader pipeline
    case node_type::text:
        return 2; // Text rendering pipeline
    case node_type::image:
        return 3; // Image rendering pipeline
    case node_type::container:
        return 0; // Container (no rendering)
    case node_type::custom:
        return 4; // Custom rendering pipeline
    default:
        return 0;
    }
}

// Helper: Check if two nodes can be batched together
// Nodes can be batched if they use the same pipeline and buffers
// This is currently unused but will be used for future optimizations
[[maybe_unused]] static bool can_batch(const scene_node* a, const scene_node* b) noexcept {
    if (a == nullptr || b == nullptr) {
        return false;
    }

    // Same node type means same pipeline
    if (a->type != b->type) {
        return false;
    }

    // Containers don't render, so don't batch
    if (a->type == node_type::container) {
        return false;
    }

    return true;
}

// Translate scene diff to GPU commands
// This implements the core GPU command backend functionality
gpu_command_result gpu_command_buffer::translate_diff(const diff_change* changes,
                                                      uint32_t change_count,
                                                      const scene_graph* current_scene) noexcept {
    if (changes == nullptr || current_scene == nullptr) {
        return gpu_command_result::invalid_input;
    }

    if (!is_valid()) {
        return gpu_command_result::out_of_memory;
    }

    // Clear previous commands
    clear();

    // Track current pipeline state to minimize pipeline changes
    uint32_t current_pipeline = 0;

    // Process each diff change and generate GPU commands
    // We optimize by:
    // 1. Batching buffer updates together
    // 2. Grouping draws by pipeline
    // 3. Minimizing pipeline state changes

    // First pass: Collect buffer updates (batch them together)
    uint32_t buffer_update_count = 0;
    for (uint32_t i = 0; i < change_count; ++i) {
        const diff_change& change = changes[i];

        switch (change.operation) {
        case diff_op::add_node:
        case diff_op::update_props: {
            // Node was added or updated - need to update GPU buffer
            const scene_node* node = current_scene->get_node(change.node);
            if (node == nullptr || node->type == node_type::container) {
                continue; // Skip containers (they don't render)
            }

            // Create buffer update for node properties
            const uint32_t buffer_id = get_buffer_id_for_node(node->id);
            const uint32_t size = sizeof(node_properties);

            // Store buffer data in our internal storage
            uint32_t data_offset = 0;
            const gpu_command_result result =
                add_buffer_data(&node->props, size, data_offset);
            if (result != gpu_command_result::success) {
                return result;
            }

            // Create buffer update command
            const gpu_command cmd = gpu_command::create_buffer_update(
                buffer_id, 0, size, buffer_data + data_offset);

            const gpu_command_result add_result = add_command(cmd);
            if (add_result != gpu_command_result::success) {
                return add_result;
            }

            buffer_update_count++;
            break;
        }
        case diff_op::remove_node:  // NOLINT(bugprone-branch-clone)
            // Node removed - no GPU update needed (we just don't draw it)
            // In a real system, we might free GPU resources here
            break;
        case diff_op::add_child:
        case diff_op::remove_child:
        case diff_op::reorder_child:
            // Child operations don't directly affect GPU state
            break;
        }
    }

    // Add sync point after buffer updates (if any)
    // This ensures buffer updates complete before drawing
    if (buffer_update_count > 0) {
        const gpu_command sync_cmd = gpu_command::create_sync_point();
        const gpu_command_result sync_result = add_command(sync_cmd);
        if (sync_result != gpu_command_result::success) {
            return sync_result;
        }
    }

    // Second pass: Generate draw calls with batching
    // We batch draws by pipeline to minimize state changes

    // Walk scene graph and generate batched draw calls
    const uint32_t scene_node_count = current_scene->count();
    bool in_batch = false;

    for (uint32_t i = 0; i < scene_node_count; ++i) {
        const scene_node* node = current_scene->get_node_by_index(i);
        if (node == nullptr || !node->is_valid() || node->type == node_type::container) {
            continue; // Skip invalid nodes and containers (they don't render)
        }

        const uint32_t pipeline_id = get_pipeline_id_for_node(node->type);

        // Check if we need to switch pipeline
        if (pipeline_id != current_pipeline) {
            // End previous batch if one is active
            if (in_batch) {
                const gpu_command end_batch = gpu_command::create_end_batch();
                const gpu_command_result result = add_command(end_batch);
                if (result != gpu_command_result::success) {
                    return result;
                }
            }

            // Set new pipeline
            const uint32_t buffer_id = get_buffer_id_for_node(node->id);
            const gpu_command pipeline_cmd =
                gpu_command::create_set_pipeline(pipeline_id, buffer_id, 0, 0);
            const gpu_command_result result = add_command(pipeline_cmd);
            if (result != gpu_command_result::success) {
                return result;
            }

            current_pipeline = pipeline_id;

            // Begin new batch
            // Note: Batch size is set to 1 as a placeholder. In a production implementation,
            // this would be calculated based on the number of consecutive nodes with the
            // same pipeline state. The current implementation generates one batch per
            // pipeline switch, which demonstrates the batching structure.
            const gpu_command begin_batch = gpu_command::create_begin_batch(1);
            const gpu_command_result batch_result = add_command(begin_batch);
            if (batch_result != gpu_command_result::success) {
                return batch_result;
            }
            in_batch = true;
        }

        // Add draw call
        // For rectangles, we draw 6 indices (2 triangles)
        const uint32_t index_count = 6;
        const gpu_command draw_cmd = gpu_command::create_draw_indexed(index_count, 1, 0, 0, 0);
        const gpu_command_result draw_result = add_command(draw_cmd);
        if (draw_result != gpu_command_result::success) {
            return draw_result;
        }
    }

    // End final batch if one is active
    if (in_batch) {
        const gpu_command end_batch = gpu_command::create_end_batch();
        const gpu_command_result result = add_command(end_batch);
        if (result != gpu_command_result::success) {
            return result;
        }
    }

    return gpu_command_result::success;
}

} // namespace aegis::core::frame
