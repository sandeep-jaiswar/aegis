#include "core/frame/scene_graph.hpp"
#include "core/memory/arena_allocator.hpp"

#include <cinttypes>
#include <cstdio>

using namespace aegis::core::frame;
using namespace aegis::core::memory;

// Demonstration of retained-mode scene graph with stable node IDs
int main() {
    printf("=== Aegis Scene Graph Demo ===\n\n");

    // Create arena allocator for scene graph memory
    constexpr size_t arena_size = 1024 * 1024; // 1 MB
    void* arena_memory = ::operator new(arena_size);
    arena_allocator arena(arena_memory, arena_size);

    printf("1. Creating scene graph with arena allocator\n");
    printf("   Arena size: %zu bytes\n", arena_size);

    // Configure scene graph capacity
    scene_graph_config config{.max_nodes = 100, .max_children = 200};

    // Create scene graph
    scene_graph graph(config, &arena);

    if (!graph.is_valid()) {
        printf("   ERROR: Failed to create scene graph\n");
        ::operator delete(arena_memory);
        return 1;
    }

    printf("   ✓ Scene graph created\n");
    printf("   Node capacity: %u\n", graph.capacity());
    printf("   Initial nodes: %u (root node)\n\n", graph.count());

    // Create some nodes
    printf("2. Creating scene nodes (Frame 1)\n");

    // Create container for UI
    node_properties container_props{
        .x = 0.0F,
        .y = 0.0F,
        .width = 800.0F,
        .height = 600.0F,
        .color = 0xFFFFFFFF // White background
    };
    node_id container = graph.create_node(node_type::container, container_props);
    printf("   Created container node: ID=%" PRIu64 "\n", container);

    // Create rectangle
    node_properties rect_props{
        .x = 10.0F,
        .y = 10.0F,
        .width = 100.0F,
        .height = 50.0F,
        .color = 0xFF0000FF // Red
    };
    node_id rect = graph.create_node(node_type::rectangle, rect_props);
    printf("   Created rectangle node: ID=%" PRIu64 "\n", rect);

    // Create text node
    node_properties text_props{
        .x = 10.0F,
        .y = 70.0F,
        .width = 200.0F,
        .height = 30.0F,
        .color = 0x000000FF // Black
    };
    node_id text = graph.create_node(node_type::text, text_props);
    printf("   Created text node: ID=%" PRIu64 "\n", text);

    printf("   Total nodes: %u\n\n", graph.count());

    // Build scene hierarchy
    printf("3. Building scene hierarchy\n");

    // Add container to root
    scene_result result = graph.add_child(root_node_id, container);
    if (result == scene_result::success) {
        printf("   ✓ Added container to root\n");
    } else {
        printf("   ERROR: Failed to add container to root\n");
    }

    // Add rectangle and text to container
    result = graph.add_child(container, rect);
    if (result == scene_result::success) {
        printf("   ✓ Added rectangle to container\n");
    }

    result = graph.add_child(container, text);
    if (result == scene_result::success) {
        printf("   ✓ Added text to container\n");
    }

    printf("\n");

    // Freeze scene graph (simulate frame execution)
    printf("4. Freezing scene graph (simulate frame execution)\n");
    graph.freeze();
    printf("   ✓ Scene graph frozen (immutable)\n");
    printf("   Frozen status: %s\n\n", graph.frozen() ? "true" : "false");

    // Attempt to modify frozen graph (should fail)
    printf("5. Testing immutability (should fail)\n");
    node_properties test_props{};
    node_id invalid = graph.create_node(node_type::rectangle, test_props);
    if (invalid == invalid_node_id) {
        printf("   ✓ Cannot create nodes while frozen (as expected)\n");
    } else {
        printf("   ERROR: Created node while frozen (unexpected!)\n");
    }

    result = graph.add_child(container, invalid);
    if (result == scene_result::graph_frozen) {
        printf("   ✓ Cannot add children while frozen (as expected)\n\n");
    } else {
        printf("   ERROR: Added child while frozen (unexpected!)\n\n");
    }

    // Traverse scene graph (read-only during frozen state)
    printf("6. Traversing frozen scene graph\n");
    const scene_node* root = graph.get_root();
    if (root != nullptr) {
        printf("   Root node: ID=%" PRIu64 ", children=%u\n", root->id, root->child_count);

        // Get root's children
        uint32_t child_count = 0;
        const node_id* root_children = graph.get_children(root->id, child_count);
        for (uint32_t i = 0; i < child_count; ++i) {
            const scene_node* child = graph.get_node(root_children[i]);
            if (child != nullptr) {
                printf("     Child %u: ID=%" PRIu64 ", type=%u, pos=(%.1f, %.1f), "
                       "size=(%.1fx%.1f)\n",
                       i, child->id, static_cast<uint32_t>(child->type), child->props.x,
                       child->props.y, child->props.width, child->props.height);

                // Get container's children
                uint32_t sub_child_count = 0;
                const node_id* sub_children = graph.get_children(child->id, sub_child_count);
                for (uint32_t j = 0; j < sub_child_count; ++j) {
                    const scene_node* sub_child = graph.get_node(sub_children[j]);
                    if (sub_child != nullptr) {
                        printf("       Subchild %u: ID=%" PRIu64 ", type=%u, pos=(%.1f, %.1f)\n", j,
                               sub_child->id, static_cast<uint32_t>(sub_child->type),
                               sub_child->props.x, sub_child->props.y);
                    }
                }
            }
        }
    }
    printf("\n");

    // Unfreeze for next frame
    printf("7. Unfreezing scene graph (prepare for next frame)\n");
    graph.unfreeze();
    printf("   ✓ Scene graph unfrozen\n");
    printf("   Frozen status: %s\n\n", graph.frozen() ? "true" : "false");

    // Demonstrate stable node IDs across frames
    printf("8. Frame 2: Creating new nodes (stable ID test)\n");
    node_properties new_rect_props{
        .x = 120.0F,
        .y = 10.0F,
        .width = 100.0F,
        .height = 50.0F,
        .color = 0x00FF00FF // Green
    };
    node_id new_rect = graph.create_node(node_type::rectangle, new_rect_props);
    printf("   Created new rectangle: ID=%" PRIu64 "\n", new_rect);
    printf("   Notice: Node ID continues from previous frame (stable IDs)\n");
    printf("   Previous node IDs are still valid:\n");

    // Verify old node IDs are still valid
    const scene_node* old_container = graph.get_node(container);
    if (old_container != nullptr) {
        printf("     Container from frame 1: ID=%" PRIu64 " still valid ✓\n", old_container->id);
    }

    const scene_node* old_rect = graph.get_node(rect);
    if (old_rect != nullptr) {
        printf("     Rectangle from frame 1: ID=%" PRIu64 " still valid ✓\n", old_rect->id);
    }

    printf("\n");

    // Memory statistics
    printf("9. Memory statistics\n");
    printf("   Total nodes: %u / %u\n", graph.count(), graph.capacity());
    printf("   Arena bytes allocated: %zu\n", arena.bytes_allocated());
    printf("   Arena bytes freed: %zu\n", arena.bytes_freed());

    printf("\n=== Demo Complete ===\n");

    // Cleanup
    ::operator delete(arena_memory);

    return 0;
}
