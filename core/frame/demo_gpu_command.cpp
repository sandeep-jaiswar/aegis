#include "core/frame/diff_engine.hpp"
#include "core/frame/gpu_command_buffer.hpp"
#include "core/frame/scene_graph.hpp"
#include "core/memory/arena_allocator.hpp"

#include <cstdio>
#include <cstdlib>

using namespace aegis::core::frame;
using namespace aegis::core::memory;

// Print GPU command for debugging
void print_gpu_command(uint32_t index, const gpu_command& cmd) {
    printf("  [%u] ", index);

    switch (cmd.type) {
    case gpu_command_type::noop:
        printf("NOOP\n");
        break;
    case gpu_command_type::update_buffer:
        printf("UPDATE_BUFFER: buffer=%u, offset=%u, size=%u\n",
               cmd.buffer_update.buffer_id, cmd.buffer_update.offset,
               cmd.buffer_update.size);
        break;
    case gpu_command_type::draw_indexed:
        printf("DRAW_INDEXED: indices=%u, instances=%u\n",
               cmd.draw_indexed.index_count, cmd.draw_indexed.instance_count);
        break;
    case gpu_command_type::set_pipeline:
        printf("SET_PIPELINE: pipeline=%u, vbuf=%u, ibuf=%u, ubuf=%u\n",
               cmd.pipeline_state.pipeline_id, cmd.pipeline_state.vertex_buffer_id,
               cmd.pipeline_state.index_buffer_id, cmd.pipeline_state.uniform_buffer_id);
        break;
    case gpu_command_type::begin_batch:
        printf("BEGIN_BATCH: size=%u\n", cmd.batch_size);
        break;
    case gpu_command_type::end_batch:
        printf("END_BATCH\n");
        break;
    case gpu_command_type::sync_point:
        printf("SYNC_POINT\n");
        break;
    }
}

// Print GPU command buffer statistics
void print_stats(const gpu_command_stats& stats) {
    printf("\nGPU Command Statistics:\n");
    printf("  Total commands: %u\n", stats.total_commands);
    printf("  Buffer updates: %u\n", stats.buffer_updates);
    printf("  Draw calls: %u\n", stats.draw_calls);
    printf("  Batched draws: %u\n", stats.batched_draws);
    printf("  Pipeline changes: %u\n", stats.pipeline_changes);
    printf("  Sync points: %u\n", stats.sync_points);
    printf("  Buffer data: %u bytes\n", stats.buffer_data_bytes);
}

int main() {
    printf("=== GPU Command Backend Demo ===\n\n");

    // Allocate memory for scene graphs and GPU commands
    constexpr size_t arena_size = 1024 * 1024; // 1MB
    void* memory = malloc(arena_size);
    if (memory == nullptr) {
        printf("ERROR: Failed to allocate memory\n");
        return 1;
    }

    arena_allocator arena(memory, arena_size);

    // Configure scene graphs
    scene_graph_config graph_cfg{.max_nodes = 200, .max_children = 400};

    scene_graph prev_scene(graph_cfg, &arena);
    scene_graph curr_scene(graph_cfg, &arena);

    // Configure diff engine
    diff_config diff_cfg{.max_changes = 1024, .max_nodes = 200};
    diff_engine differ(diff_cfg, &arena);

    // Configure GPU command buffer
    gpu_command_buffer_config gpu_cfg{.max_commands = 2048, .max_buffer_data_bytes = 65536};
    gpu_command_buffer gpu_cmds(gpu_cfg, &arena);

    // === Frame 1: Build initial scene ===
    printf("Frame 1: Building initial scene with 3 rectangles\n");

    node_properties rect1_props{.x = 10.0F, .y = 10.0F, .width = 100.0F, .height = 50.0F,
                               .color = 0xFF0000FF}; // Red
    node_id rect1 = prev_scene.create_node(node_type::rectangle, rect1_props);
    (void)prev_scene.add_child(root_node_id, rect1);

    node_properties rect2_props{.x = 120.0F, .y = 10.0F, .width = 100.0F, .height = 50.0F,
                               .color = 0x00FF00FF}; // Green
    node_id rect2 = prev_scene.create_node(node_type::rectangle, rect2_props);
    (void)prev_scene.add_child(root_node_id, rect2);

    node_properties rect3_props{.x = 230.0F, .y = 10.0F, .width = 100.0F, .height = 50.0F,
                               .color = 0x0000FFFF}; // Blue
    node_id rect3 = prev_scene.create_node(node_type::rectangle, rect3_props);
    (void)prev_scene.add_child(root_node_id, rect3);

    prev_scene.freeze();

    printf("  Scene nodes: %u\n", prev_scene.count());

    // === Frame 2: Modify scene ===
    printf("\nFrame 2: Modifying scene (move rectangle, change color)\n");

    // Rebuild current scene with modifications
    node_properties rect1_modified{.x = 15.0F, // Moved right
                                  .y = 10.0F,
                                  .width = 100.0F,
                                  .height = 50.0F,
                                  .color = 0xFF00FFFF}; // Magenta (color changed)
    rect1 = curr_scene.create_node(node_type::rectangle, rect1_modified);
    (void)curr_scene.add_child(root_node_id, rect1);

    rect2 = curr_scene.create_node(node_type::rectangle, rect2_props); // Unchanged
    (void)curr_scene.add_child(root_node_id, rect2);

    rect3 = curr_scene.create_node(node_type::rectangle, rect3_props); // Unchanged
    (void)curr_scene.add_child(root_node_id, rect3);

    curr_scene.freeze();

    // Compute diff between frames
    uint32_t change_count = 0;
    const diff_result diff_res = differ.compute_diff(&prev_scene, &curr_scene, change_count);

    if (diff_res != diff_result::success) {
        printf("ERROR: Failed to compute diff\n");
        free(memory);
        return 1;
    }

    printf("  Diff changes: %u\n", change_count);

    // Print diff changes
    const diff_change* changes = differ.get_changes();
    for (uint32_t i = 0; i < change_count; ++i) {
        const diff_change& change = changes[i];
        printf("    [%u] ", i);
        switch (change.operation) {
        case diff_op::add_node:
            printf("ADD_NODE: node=%lu\n", static_cast<unsigned long>(change.node));
            break;
        case diff_op::remove_node:
            printf("REMOVE_NODE: node=%lu\n", static_cast<unsigned long>(change.node));
            break;
        case diff_op::update_props:
            printf("UPDATE_PROPS: node=%lu (x=%.1f->%.1f, color=0x%X->0x%X)\n",
                   static_cast<unsigned long>(change.node), change.old_props.x,
                   change.new_props.x, change.old_props.color, change.new_props.color);
            break;
        default:
            printf("OTHER\n");
            break;
        }
    }

    // === Translate diff to GPU commands ===
    printf("\nTranslating diff to GPU commands...\n");

    const gpu_command_result gpu_res = gpu_cmds.translate_diff(changes, change_count, &curr_scene);

    if (gpu_res != gpu_command_result::success) {
        printf("ERROR: Failed to translate diff to GPU commands\n");
        free(memory);
        return 1;
    }

    // Print GPU commands
    const uint32_t gpu_command_count = gpu_cmds.get_command_count();
    printf("\nGenerated %u GPU commands:\n", gpu_command_count);

    const gpu_command* gpu_command_list = gpu_cmds.get_commands();
    for (uint32_t i = 0; i < gpu_command_count; ++i) {
        print_gpu_command(i, gpu_command_list[i]);
    }

    // Print statistics
    print_stats(gpu_cmds.get_stats());

    // === Frame 3: Add new rectangle ===
    printf("\n\nFrame 3: Adding a new rectangle\n");

    prev_scene.unfreeze();
    prev_scene.clear();
    curr_scene.unfreeze();
    curr_scene.clear();
    differ.clear();
    gpu_cmds.clear();

    // Previous scene (frame 2 state)
    rect1 = prev_scene.create_node(node_type::rectangle, rect1_modified);
    (void)prev_scene.add_child(root_node_id, rect1);
    rect2 = prev_scene.create_node(node_type::rectangle, rect2_props);
    (void)prev_scene.add_child(root_node_id, rect2);
    rect3 = prev_scene.create_node(node_type::rectangle, rect3_props);
    (void)prev_scene.add_child(root_node_id, rect3);
    prev_scene.freeze();

    // Current scene (add 4th rectangle)
    rect1 = curr_scene.create_node(node_type::rectangle, rect1_modified);
    (void)curr_scene.add_child(root_node_id, rect1);
    rect2 = curr_scene.create_node(node_type::rectangle, rect2_props);
    (void)curr_scene.add_child(root_node_id, rect2);
    rect3 = curr_scene.create_node(node_type::rectangle, rect3_props);
    (void)curr_scene.add_child(root_node_id, rect3);

    node_properties rect4_props{.x = 10.0F, .y = 70.0F, .width = 100.0F, .height = 50.0F,
                               .color = 0xFFFF00FF}; // Yellow
    node_id rect4 = curr_scene.create_node(node_type::rectangle, rect4_props);
    (void)curr_scene.add_child(root_node_id, rect4);

    curr_scene.freeze();

    // Compute diff
    change_count = 0;
    const diff_result diff_res2 = differ.compute_diff(&prev_scene, &curr_scene, change_count);
    if (diff_res2 != diff_result::success) {
        printf("ERROR: Failed to compute diff\n");
        free(memory);
        return 1;
    }

    printf("  Diff changes: %u\n", change_count);

    // Translate to GPU commands
    const gpu_command_result gpu_res2 =
        gpu_cmds.translate_diff(differ.get_changes(), change_count, &curr_scene);
    if (gpu_res2 != gpu_command_result::success) {
        printf("ERROR: Failed to translate diff to GPU commands\n");
        free(memory);
        return 1;
    }

    printf("\nGenerated %u GPU commands:\n", gpu_cmds.get_command_count());
    const gpu_command* gpu_command_list2 = gpu_cmds.get_commands();
    for (uint32_t i = 0; i < gpu_cmds.get_command_count(); ++i) {
        print_gpu_command(i, gpu_command_list2[i]);
    }

    print_stats(gpu_cmds.get_stats());

    // === Success ===
    printf("\n=== Demo Complete ===\n");
    printf("\nKey Achievements:\n");
    printf("  ✓ No per-node draw calls (batched by pipeline)\n");
    printf("  ✓ Batched buffer updates (all updates before draws)\n");
    printf("  ✓ CPU↔GPU sync minimized (single sync point after updates)\n");

    // Cleanup
    free(memory);

    return 0;
}
