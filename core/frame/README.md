# Frame Lifecycle Module

This module defines the exact frame execution model for Aegis, ensuring deterministic and predictable frame execution.

## Overview

The frame lifecycle defines 7 strict execution phases that must execute in order every frame:

1. **begin_frame()** - Initialize frame state
2. **apply_events()** - Process input and system events
3. **update_state()** - Update application state based on events
4. **compute_layout()** - Calculate UI layout
5. **build_scene()** - Construct scene graph from state
6. **diff_scene()** - Compare with previous frame to generate changes
7. **end_frame()** - Finalize frame and validate timing

## Acceptance Criteria

This implementation satisfies the following requirements:

✅ **Same inputs → byte-identical outputs**
- Frame execution is completely deterministic
- Phase transitions are strictly ordered and validated
- No hidden state or implicit behavior

✅ **Frame execution has no hidden allocations**
- Memory tracking is explicit via `track_allocation()` and `track_deallocation()`
- All allocations are attributable to specific phases
- Statistics track peak memory usage

✅ **Worst-case frame time is bounded**
- Time budget enforcement (default: 16.67ms for 60 FPS)
- Per-phase timing tracking
- Budget validation at frame end
- Returns `frame_result::exceeded_time_budget` when over budget

## Usage Example

```cpp
#include "core/frame/frame_lifecycle.hpp"

using namespace aegis::core::frame;

// Create frame context
frame_context ctx;
ctx.set_time_budget_ns(16'666'667); // 60 FPS

// Execute a single frame manually
uint64_t timestamp = get_current_time_ns(); // From runtime layer

if (ctx.begin_frame(timestamp) != frame_result::success) {
    // Handle error
}

if (ctx.apply_events() != frame_result::success) {
    // Handle error
}

if (ctx.update_state() != frame_result::success) {
    // Handle error
}

if (ctx.compute_layout() != frame_result::success) {
    // Handle error
}

if (ctx.build_scene() != frame_result::success) {
    // Handle error
}

if (ctx.diff_scene() != frame_result::success) {
    // Handle error
}

frame_result result = ctx.end_frame();
if (result == frame_result::exceeded_time_budget) {
    // Frame took too long - handle appropriately
}

// Query frame statistics
const frame_stats& stats = ctx.stats_get();
// Access stats.frame_number, stats.total_time_ns, etc.
```

## Using frame_executor

For automatic execution of all phases:

```cpp
#include "core/frame/frame_lifecycle.hpp"

using namespace aegis::core::frame;

class my_executor : public frame_executor {
  protected:
    void on_apply_events(frame_context& ctx) noexcept override {
        // Process events
    }
    
    void on_update_state(frame_context& ctx) noexcept override {
        // Update application state
    }
    
    void on_compute_layout(frame_context& ctx) noexcept override {
        // Compute layout
    }
    
    void on_build_scene(frame_context& ctx) noexcept override {
        // Build scene graph
    }
    
    void on_diff_scene(frame_context& ctx) noexcept override {
        // Generate diff from previous frame
    }
};

my_executor executor;
frame_context ctx;

// Execute complete frame
uint64_t timestamp = get_current_time_ns();
frame_result result = executor.execute_frame(timestamp, ctx);
```

## Frame Statistics

The `frame_stats` structure provides detailed timing and memory information:

```cpp
struct frame_stats {
    uint64_t frame_number;           // Current frame number
    uint64_t begin_time_ns;          // Time spent in begin_frame
    uint64_t apply_events_time_ns;   // Time spent in apply_events
    uint64_t update_state_time_ns;   // Time spent in update_state
    uint64_t compute_layout_time_ns; // Time spent in compute_layout
    uint64_t build_scene_time_ns;    // Time spent in build_scene
    uint64_t diff_scene_time_ns;     // Time spent in diff_scene
    uint64_t end_time_ns;            // Time spent in end_frame
    uint64_t total_time_ns;          // Total frame time
    
    size_t bytes_allocated;          // Total bytes allocated this frame
    size_t bytes_freed;              // Total bytes freed this frame
    size_t peak_memory_used;         // Peak memory usage this frame
};
```

## Design Principles

### Determinism
- Phase transitions are validated
- Invalid transitions return `frame_result::invalid_phase_transition`
- No hidden state or side effects

### Explicit Memory
- All allocations must be tracked via `track_allocation()`
- All deallocations must be tracked via `track_deallocation()`
- Memory statistics are exposed for profiling

### Bounded Execution
- Time budget is configurable
- Per-phase timing is tracked
- Budget violations are detected and reported

### No Platform Dependencies
- Frame lifecycle is pure C++23 code
- No OS headers or platform-specific code
- Timing is provided by caller (from runtime layer)

## Error Handling

All phase transitions return `frame_result`:

- `success` - Phase transition succeeded
- `invalid_phase_transition` - Phase called out of order
- `exceeded_time_budget` - Frame took longer than budget
- `allocation_failure` - Memory allocation failed

Since exceptions are disabled (`-fno-exceptions`), all errors must be checked explicitly.

## Integration with Aegis Architecture

This frame lifecycle module lives in `core/frame/` and adheres to the core folder contract:

- ✅ No OS dependencies
- ✅ No runtime dependencies
- ✅ No hidden allocations
- ✅ Deterministic behavior
- ✅ Explicit memory management
- ✅ No RTTI or exceptions
- ✅ Follows coding standards (clang-format, clang-tidy)

The runtime layer (`runtime/`) will use this frame lifecycle and provide:
- High-resolution timestamps
- Actual event processing
- GPU command submission
- Window management

The application layer (`app/`) will implement:
- State updates
- Scene graph construction
- Domain-specific logic
