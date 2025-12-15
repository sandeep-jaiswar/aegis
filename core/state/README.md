# State Module

This module implements the immutable state model for Aegis, providing snapshot-based state transitions with explicit structural sharing.

## Overview

The state module satisfies the following acceptance criteria:

1. **Previous state remains valid after update** - All state snapshots are immutable value types
2. **State transition is a pure function** - Transitions take `(old_state, event) -> new_state` with no side effects
3. **Structural sharing is explicit and bounded** - Shared data has visible reference counts and explicit copy-on-write

## Core Concepts

### State Snapshot

A `state_snapshot<T>` represents an immutable, frozen-in-time view of application state.

```cpp
#include "core/state/state_snapshot.hpp"

using namespace aegis::core::state;

// Define your state type
struct app_state {
    int counter;
    float value;
};

// Create initial snapshot
state_snapshot<app_state> s0({0, 0.0f}, 0);

// Snapshots are immutable - can only read data
const app_state& data = s0.data();
```

#### Features

- **Immutability**: Data is read-only through `const` interface
- **Versioning**: Each snapshot has a monotonic version number
- **Value semantics**: Snapshots can be copied freely
- **No hidden state**: All state is explicit in the snapshot

### State Transition

A `state_transition<StateData, EventData>` applies events to states using pure functions.

```cpp
#include "core/state/state_transition.hpp"

using namespace aegis::core::state;

// Define event type
struct increment_event {
    int amount;
};

// Define pure transition function
state_snapshot<app_state>
handle_increment(const state_snapshot<app_state>& current,
                 const state_event<increment_event>& event,
                 transition_result& result) noexcept {
    // Read current state (immutable)
    const app_state& old_state = current.data();
    
    // Create new state (no modification of old)
    app_state new_state = old_state;
    new_state.counter += event.data.amount;
    
    // Return new snapshot
    result = transition_result::success;
    return state_snapshot<app_state>(new_state, current.version() + 1);
}

// Use the transition
state_transition<app_state, increment_event> transition(handle_increment);

state_snapshot<app_state> s0({0, 0.0f}, 0);
state_event<increment_event> event({5});

transition_result result;
state_snapshot<app_state> s1 = transition.apply(s0, event, result);

// s0 remains valid and unchanged
// s1 contains the new state with counter = 5
```

#### Features

- **Pure functions**: Transitions have no side effects
- **Deterministic**: Same inputs always produce same outputs
- **Non-throwing**: Uses result codes instead of exceptions
- **Batch operations**: Apply multiple events in sequence

### Structural Sharing

The `shared_data<T>` type provides explicit copy-on-write sharing between snapshots.

```cpp
#include "core/state/structural_sharing.hpp"
#include "core/memory/arena_allocator.hpp"

using namespace aegis::core::state;
using namespace aegis::core::memory;

// State with large data that benefits from sharing
struct large_state {
    shared_data<std::array<int, 1000>> data;
};

// Create shared data using an allocator
arena_allocator alloc(buffer, buffer_size);
auto shared = shared_data<std::array<int, 1000>>::create(array_data, &alloc);

// Copy shares the data (reference count incremented)
auto shared2 = shared;

// Reference count is explicit
uint32_t refs = shared.ref_count(); // Returns 2

// Check if we can modify (only if unique owner)
if (shared.is_unique()) {
    int* mut_data = shared.get_mut(); // Can modify
} else {
    auto cloned = shared.clone(); // Explicit copy-on-write
    int* mut_data = cloned.get_mut(); // Now unique, can modify
}

// Calculate sharing statistics
sharing_stats stats = calculate_sharing_stats(shared);
float ratio = stats.sharing_ratio(); // Percentage of shared data
```

#### Features

- **Explicit reference counting**: No hidden behavior
- **Copy-on-write**: Explicit `clone()` operation
- **Bounded lifetime**: Tied to allocator lifetime
- **No std::shared_ptr**: Custom implementation for deterministic behavior
- **Statistics**: Track sharing ratios and memory usage

## Design Principles

### Immutability

All state snapshots are immutable after creation:

- **No setters**: State data is `const`
- **No in-place updates**: Transitions create new snapshots
- **Previous states remain valid**: Old snapshots unchanged

### Purity

State transitions are pure functions:

- **No side effects**: Cannot modify global state
- **Deterministic**: Same inputs → same outputs
- **Composable**: Can chain transitions

### Explicitness

All operations are explicit:

- **Visible allocations**: Allocator must be provided
- **Explicit sharing**: `clone()` for copy-on-write
- **Tracked metadata**: Version, timestamp, sharing stats

### No Platform Dependencies

The state module follows the core folder contract:

- ✅ No OS dependencies
- ✅ No runtime dependencies
- ✅ No exceptions (uses result codes)
- ✅ No RTTI
- ✅ No hidden allocations
- ✅ Deterministic behavior

## Integration with Aegis Architecture

### Frame Lifecycle

State transitions integrate with the frame lifecycle:

```cpp
#include "core/frame/frame_lifecycle.hpp"
#include "core/state/state_transition.hpp"

// In the update_state phase
void on_update_state(frame_context& ctx) noexcept {
    // Apply events to current state
    auto new_state = transition.apply(current_state, event, result);
    
    if (result == transition_result::success) {
        // Update to new state
        current_state = new_state;
    }
}
```

### Memory Management

State uses explicit allocators from the memory module:

```cpp
#include "core/memory/arena_allocator.hpp"

// Use arena allocator for state snapshots
arena_allocator state_arena(buffer, buffer_size);

// Shared data tied to allocator lifetime
auto shared = shared_data<MyData>::create(data, &state_arena);

// Arena reset invalidates all shared data
state_arena.reset(); // All shared_data from this arena are now invalid
```

## Usage Examples

### Simple Counter

```cpp
struct counter_state {
    int value;
};

struct increment {};

state_snapshot<counter_state>
handle_event(const state_snapshot<counter_state>& current,
             const state_event<increment>& event,
             transition_result& result) noexcept {
    counter_state new_state{current.data().value + 1};
    result = transition_result::success;
    return state_snapshot<counter_state>(new_state, current.version() + 1);
}
```

### State with Structural Sharing

```cpp
struct app_state {
    shared_data<std::string> config; // Shared across snapshots
    int frame_counter;               // Updated each frame
};

state_snapshot<app_state>
handle_frame(const state_snapshot<app_state>& current,
             const state_event<frame_event>& event,
             transition_result& result) noexcept {
    app_state new_state = current.data();
    
    // Config is shared (no copy)
    // frame_counter is copied (cheap)
    new_state.frame_counter++;
    
    result = transition_result::success;
    return state_snapshot<app_state>(new_state, current.version() + 1);
}
```

### Tracked Transitions with Metadata

```cpp
tracked_transition<app_state, app_event> tracked(handle_event);

tracked_snapshot<app_state> current = /* ... */;
state_event<app_event> event = /* ... */;

transition_result result;
tracked_snapshot<app_state> next = tracked.apply(current, event, result);

// Access metadata
const snapshot_metadata& meta = next.metadata();
state_version version = meta.version;
state_version parent = meta.parent_version;
float sharing = meta.sharing_ratio();
```

## Error Handling

All operations return result codes (no exceptions):

```cpp
transition_result result;
auto new_state = transition.apply(current, event, result);

switch (result) {
    case transition_result::success:
        // Transition succeeded
        break;
    case transition_result::invalid_event:
        // Event validation failed
        break;
    case transition_result::allocation_failed:
        // Memory allocation failed
        break;
    // Handle other cases...
}
```

## Performance Characteristics

- **Snapshot creation**: O(1) for small states, O(n) for deep copies
- **Structural sharing**: O(1) copy with reference counting
- **Transition application**: O(1) function call overhead
- **Batch transitions**: O(n) where n = number of events
- **Memory overhead**: One version number + metadata per snapshot

## Testing

The state module can be tested deterministically:

```cpp
// Same inputs → same outputs
state_snapshot<S> s0 = initial_state();
state_event<E> e = create_event();

transition_result r1, r2;
auto s1 = transition.apply(s0, e, r1);
auto s2 = transition.apply(s0, e, r2);

// s1 and s2 are identical (deterministic)
assert(s1.data() == s2.data());
assert(s1.version() == s2.version());
```

## Acceptance Criteria Validation

### 1. Previous state remains valid after update ✅

```cpp
state_snapshot<S> s0 = initial;
state_snapshot<S> s1 = transition.apply(s0, event, result);

// s0 is still valid and unchanged
assert(s0.data() == original_data);
assert(s0.version() == 0);
```

### 2. State transition is a pure function ✅

```cpp
// Pure function signature
state_snapshot<S> transition(
    const state_snapshot<S>& current,  // Immutable input
    const state_event<E>& event,       // Immutable input
    transition_result& result          // Output parameter
) noexcept;                            // No exceptions

// No side effects, no global state modification
// Deterministic: same inputs → same outputs
```

### 3. Structural sharing is explicit and bounded ✅

```cpp
// Reference count is visible
uint32_t refs = shared.ref_count();

// Sharing is explicit through copy
auto shared2 = shared; // Explicit share

// Clone is explicit
auto independent = shared.clone(); // Explicit copy

// Bounded by allocator lifetime
arena_allocator alloc(buffer, size);
auto data = shared_data<T>::create(value, &alloc);
alloc.reset(); // All shared_data from alloc are invalidated
```
