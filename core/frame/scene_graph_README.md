# Scene Graph Module

This module implements a **retained-mode scene graph** with stable node IDs and immutable frame semantics.

## Overview

The scene graph provides a persistent, hierarchical representation of renderable elements that:

1. **Remains stable during frame execution** (immutable from `build_scene` to `end_frame`)
2. **Maintains stable node IDs across frames** (IDs never reused, monotonically increasing)
3. **Prevents self-mutation** (nodes cannot modify themselves during frame execution)

## Acceptance Criteria

This implementation satisfies the following requirements:

✅ **Scene graph is immutable during frame**
- Graph can be frozen via `freeze()` to prevent modifications
- Frozen graphs reject all mutation operations
- Nodes are marked as frozen individually for safety

✅ **Node IDs are stable across frames**
- Node IDs are monotonically increasing (`next_id++`)
- IDs are never reused within the same session
- Old node IDs remain valid after unfreezing for next frame

✅ **No self-mutating nodes**
- All node properties are const during frozen state
- Node structure is immutable (POD types only)
- No hidden state or callbacks that could mutate nodes

## Core Types

### `node_id`

Stable identifier for scene nodes. IDs are `uint64_t` values that:
- Start from `root_node_id = 1`
- Increment monotonically
- Never wrap or reuse values
- Use `invalid_node_id = 0` for null references

### `node_type`

Classification of scene node:
- `container` - Layout container that can have children
- `rectangle` - Rectangle primitive
- `text` - Text primitive
- `image` - Image primitive
- `custom` - Application-defined type

### `node_properties`

Immutable properties for each node:
```cpp
struct node_properties {
    float x, y;          // Position
    float width, height; // Size
    uint32_t color;      // RGBA color
};
```

All properties are POD types for deterministic behavior and GPU compatibility.

### `scene_node`

Immutable scene node structure:
```cpp
struct scene_node {
    node_id id;                  // Stable identifier
    node_id parent_id;           // Parent reference
    node_type type;              // Node classification
    node_properties props;       // Frozen properties
    uint32_t first_child_index;  // Child range start
    uint32_t child_count;        // Number of children
    bool frozen;                 // Immutability flag
};
```

Nodes use index-based child storage for:
- O(1) child access
- Cache-friendly traversal
- No pointer indirection

## Scene Graph API

### Construction

```cpp
#include "core/frame/scene_graph.hpp"

using namespace aegis::core::frame;

// Configure capacity
scene_graph_config config{
    .max_nodes = 1024,
    .max_children = 2048
};

// Create with allocator
scene_graph graph(config, &my_allocator);
```

### Node Creation

```cpp
// Create node with properties
node_properties props{
    .x = 10.0F,
    .y = 10.0F,
    .width = 100.0F,
    .height = 50.0F,
    .color = 0xFF0000FF // Red
};

node_id rect = graph.create_node(node_type::rectangle, props);
```

### Building Hierarchy

```cpp
// Add child to parent
scene_result result = graph.add_child(parent_id, child_id);
if (result != scene_result::success) {
    // Handle error
}
```

**Important:** Children must be added to each parent in a single contiguous sequence. 
You cannot interleave adding children to different parents. This ensures the child 
array remains contiguous per parent for cache-friendly traversal.

✅ Correct:
```cpp
// Add all children to parent A
graph.add_child(parent_a, child1);
graph.add_child(parent_a, child2);
// Then add all children to parent B
graph.add_child(parent_b, child3);
graph.add_child(parent_b, child4);
```

❌ Incorrect:
```cpp
// Don't interleave different parents
graph.add_child(parent_a, child1);
graph.add_child(parent_b, child2);  // Error: breaks contiguity
graph.add_child(parent_a, child3);  // This will fail!
```

### Frame Lifecycle Integration

```cpp
// During build_scene phase
void on_build_scene(frame_context& ctx) {
    // Build scene graph
    node_id container = graph.create_node(node_type::container, props);
    graph.add_child(root_node_id, container);
    
    // Freeze for immutability during rendering
    graph.freeze();
}

// During diff_scene phase (graph is frozen)
void on_diff_scene(frame_context& ctx) {
    // Traverse frozen graph (read-only)
    const scene_node* root = graph.get_root();
    uint32_t child_count;
    const node_id* children = graph.get_children(root->id, child_count);
    
    // Generate rendering commands...
}

// During end_frame or next begin_frame
void prepare_next_frame() {
    // Unfreeze for next frame
    graph.unfreeze();
    
    // Optionally clear (preserves stable IDs)
    graph.clear();
}
```

### Read-Only Access

```cpp
// Get node by ID
const scene_node* node = graph.get_node(node_id);

// Get root node
const scene_node* root = graph.get_root();

// Get children
uint32_t count;
const node_id* children = graph.get_children(parent_id, count);
for (uint32_t i = 0; i < count; ++i) {
    const scene_node* child = graph.get_node(children[i]);
    // Process child...
}
```

## Memory Management

The scene graph uses explicit memory management:

- **No hidden allocations** - All memory comes from provided allocator
- **Predictable layout** - Two arrays: nodes and child relationships
- **Bounded capacity** - Fixed limits set at construction time
- **Frame allocator compatible** - Works with frame allocators for O(1) cleanup

### Memory Layout

```
Nodes Array:
[root][node1][node2][node3]...

Children Array:
[child_id][child_id][child_id]...
           ↑
           first_child_index from parent node
```

## Error Handling

All operations return result codes (no exceptions):

```cpp
enum class scene_result : uint8_t {
    success = 0,
    out_of_memory = 1,
    invalid_node_id = 2,
    node_limit_exceeded = 3,
    child_limit_exceeded = 4,
    graph_frozen = 5,
    parent_not_found = 6
};
```

## Design Principles

### 1. Determinism
- Same inputs → identical scene graph structure
- No non-deterministic behavior
- Predictable memory layout

### 2. Immutability
- Freeze/unfreeze lifecycle enforced
- No modification during frozen state
- Const-correct API

### 3. Stability
- Node IDs never reused
- IDs remain valid across frames
- Monotonic ID allocation

### 4. Explicitness
- No hidden allocations
- No implicit behavior
- Clear ownership model

### 5. Performance
- Cache-friendly data layout
- O(1) freeze/unfreeze operations
- Minimal indirection

## Integration with Aegis Architecture

This scene graph implementation:

- ✅ Lives in `core/frame/` (no OS dependencies)
- ✅ Uses explicit memory management
- ✅ Has deterministic behavior
- ✅ Follows coding standards (clang-format, clang-tidy)
- ✅ Integrates with frame lifecycle
- ✅ Compatible with GPU rendering

## Example Usage

See `demo_scene_graph.cpp` for a complete working example demonstrating:

1. Scene graph creation
2. Node creation with stable IDs
3. Hierarchy building
4. Freeze/unfreeze lifecycle
5. Read-only traversal during frozen state
6. Stable IDs across multiple frames
7. Memory management patterns

Build the demo:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/core/scene_graph_demo
```

## Future Enhancements

Possible extensions (not currently implemented):

- Node ID → Index hash map for O(1) lookups (currently O(n))
- Spatial indexing for culling
- Dirty tracking for selective updates
- GPU-friendly packed representation
- Batch node creation APIs

These would be added based on profiling and actual usage patterns.
