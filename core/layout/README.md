# One-Pass Layout Engine

This module implements a deterministic, constraint-based layout engine with O(n) complexity.

## Overview

The layout engine computes positions and sizes for a tree of boxes in a single pass, ensuring:

- **O(n) complexity**: Each box is visited exactly twice (once for sizing, once for positioning)
- **Deterministic results**: Same inputs always produce identical outputs
- **No cascading recalculations**: Each box's layout is computed once
- **Constraint-based**: Flexible sizing based on explicit constraints

## Acceptance Criteria

✅ **Layout complexity is O(n)**
- Single pass algorithm: two phases (size computation + position computation)
- Each box visited exactly twice, regardless of tree depth
- No iterative refinement or cascading updates

✅ **No cascading recalculations**
- Bottom-up size computation ensures children are sized before parents
- Top-down position computation ensures parents are positioned before children
- No box is revisited once its layout is computed

✅ **Same inputs produce same layout results**
- Deterministic constraint resolution
- No hidden state or randomness
- Explicit, predictable behavior for all constraint types

## Architecture

### Key Types

#### `dimension`
Basic unit for layout measurements. Wraps a float but prevents implicit conversions.

#### `box_constraints`
Defines how a box should be sized:
- Min/max width and height bounds
- Size constraint type (fixed, min, max, fill, content)

#### `layout_box`
Represents a single element in the layout tree:
- Tree structure (parent, first child, next sibling)
- Constraints and padding
- Layout direction and alignment
- Computed position and size

#### `layout_engine`
The main layout computation engine with a single public method:
```cpp
layout_result compute_layout(layout_box* boxes,
                            size_t box_count,
                            box_id root_id,
                            size available_space);
```

## Algorithm

The layout engine uses a two-phase algorithm:

### Phase 1: Compute Sizes (Bottom-Up)

Starting from leaf nodes and moving toward the root:

1. For each box:
   - First, compute sizes for all children recursively
   - Then, compute this box's size based on:
     - Its constraints
     - Available space from parent
     - Size of children (if using `content` constraint)

2. This ensures children are always sized before their parents

### Phase 2: Compute Positions (Top-Down)

Starting from the root and moving toward leaves:

1. For each box:
   - Set its position (provided by parent)
   - Compute positions for children based on:
     - Parent's padding
     - Layout direction (horizontal/vertical)
     - Alignment settings
   - Recursively position all children

2. This ensures parents are always positioned before their children

### Complexity Analysis

- **Phase 1**: Each box visited once → O(n)
- **Phase 2**: Each box visited once → O(n)
- **Total**: O(n) + O(n) = O(n)

No box is visited more than twice, regardless of tree structure.

## Size Constraints

The engine supports five types of size constraints:

1. **`fixed`**: Use exact content size, clamped to min/max
2. **`min`**: Use at least content size, can grow to fill available space
3. **`max`**: Use at most available space, can shrink to content size
4. **`fill`**: Fill all available space (subject to min/max bounds)
5. **`content`**: Size based on children (computed automatically)

## Alignment

Children can be aligned within their parent using:

- **Horizontal**: `start`, `center`, `end`, `stretch`
- **Vertical**: `start`, `center`, `end`, `stretch`

Alignment is applied during position computation and doesn't affect the size phase.

## Usage Example

```cpp
#include "core/layout/layout_engine.hpp"

using namespace aegis::core::layout;

// Create boxes
layout_box boxes[3];

// Root box (fills available space)
boxes[0].id = 1;
boxes[0].constraints.width_constraint = size_constraint::fill;
boxes[0].constraints.height_constraint = size_constraint::fill;
boxes[0].layout_direction = direction::vertical;
boxes[0].first_child_id = 2;

// Child box (fixed size)
boxes[1].id = 2;
boxes[1].parent_id = 1;
boxes[1].content_size = size{dimension(100.0f), dimension(50.0f)};
boxes[1].constraints.width_constraint = size_constraint::fixed;
boxes[1].constraints.height_constraint = size_constraint::fixed;
boxes[1].next_sibling_id = 3;

// Another child (content-sized)
boxes[2].id = 3;
boxes[2].parent_id = 1;
boxes[2].constraints.width_constraint = size_constraint::content;
boxes[2].constraints.height_constraint = size_constraint::content;

// Compute layout
layout_engine engine;
size available{dimension(800.0f), dimension(600.0f)};
layout_result result = engine.compute_layout(boxes, 3, 1, available);

if (result.success) {
    // Access computed layouts
    for (size_t i = 0; i < result.box_count; ++i) {
        const rect& r = boxes[i].computed_rect;
        // Use r.pos.x, r.pos.y, r.sz.width, r.sz.height
    }
}
```

## Design Principles

### Determinism
- All computations use deterministic floating-point operations
- No hidden state or caches
- Same input tree always produces identical output

### Explicit Control
- No implicit behavior or magic
- All constraints are explicit
- Memory layout is predictable (POD types)

### Performance
- O(n) complexity guaranteed
- No dynamic allocation during layout
- GPU-friendly data structures

### Integration with Aegis Architecture

This layout engine adheres to the core folder contract:

- ✅ No OS dependencies
- ✅ No runtime dependencies
- ✅ No hidden allocations
- ✅ Deterministic behavior
- ✅ Explicit memory management
- ✅ No RTTI or exceptions
- ✅ POD-friendly, GPU-compatible structures

## Limitations

- **No circular dependencies**: Tree structure must be acyclic
- **No incremental updates**: Full recomputation required (but O(n) so cheap)
- **No text measurement**: Content sizes must be provided externally
- **No floating-point guarantees**: Results may vary slightly across platforms due to FP arithmetic

## Integration with Frame Lifecycle

The layout engine is designed to be called during the `compute_layout` phase of the frame lifecycle:

```cpp
class my_frame_executor : public frame_executor {
  protected:
    void on_compute_layout(frame_context& ctx) noexcept override {
        // Build layout boxes from application state
        // ...
        
        // Compute layout
        layout_result result = engine.compute_layout(boxes, count, root, available);
        
        // Store results for scene building phase
        // ...
    }
    
  private:
    layout_engine engine;
};
```

## Future Enhancements

Possible improvements (not required for acceptance criteria):

- Flexbox-style layout modes
- Grid layout support
- Aspect ratio constraints
- Caching for unchanged subtrees
- SIMD optimizations for batch operations

## References

- [Frame Lifecycle](../frame/README.md)
- [Core Folder Contract](../../docs/CORE_FOLDER_CONTRACT.md)
- [Architecture](../../docs/ARCHITECTURE.md)
