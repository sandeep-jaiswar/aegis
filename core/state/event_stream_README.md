# Event Stream Module

This module implements a deterministic event system for Aegis, providing typed, ordered event ingestion with replay capabilities.

## Overview

The event stream module satisfies the following requirements:

1. **Events processed in deterministic order** - Events are stored by sequence number and processed in that exact order
2. **Replaying event stream produces identical state** - Multiple replays of the same event stream always produce byte-identical results
3. **No callbacks or implicit dispatch** - Event processing is explicit through iteration or replay functions
4. **No format and lint issues** - Code follows Aegis coding standards (clang-format, clang-tidy)

## Core Concepts

### Event Stream

An `event_stream<EventData>` is a bounded, deterministic container for events:

```cpp
#include "core/state/event_stream.hpp"

using namespace aegis::core::state;

// Define your event type
struct my_event {
    int value;
    float factor;
};

// Create event stream with default capacity (1024 events)
event_stream<my_event> stream = make_event_stream<my_event>();

// Or with custom configuration
event_stream_config config{};
config.max_events = 2048;
config.auto_sequence = true;
event_stream<my_event> custom_stream(config);
```

#### Features

- **Deterministic ordering**: Events are totally ordered by sequence number
- **Bounded capacity**: Fixed maximum size prevents unbounded memory growth
- **Automatic sequencing**: Sequence numbers assigned automatically (optional)
- **No callbacks**: Explicit iteration and replay only
- **Statistics**: Track event counts, memory usage, replay operations

### Adding Events

Events can be added with automatic sequencing:

```cpp
uint64_t sequence;
event_stream_result result = stream.push(my_event{42, 3.14f}, timestamp_ns, &sequence);

if (result == event_stream_result::success) {
    // Event added successfully, sequence number returned
}
```

Or with explicit metadata:

```cpp
event_metadata meta{};
meta.timestamp_ns = get_current_time();
meta.sequence = 100;
meta.event_type = 1;

state_event<my_event> event(meta, my_event{42, 3.14f});
event_stream_result result = stream.push(event);
```

### Replaying Events

The key feature is deterministic replay - applying all events to produce a final state:

```cpp
// Define state and transition function
struct app_state {
    int counter;
};

state_snapshot<app_state> transition_fn(
    const state_snapshot<app_state>& current,
    const state_event<my_event>& event,
    transition_result& result) noexcept {
    
    app_state new_state = current.data();
    new_state.counter += event.data.value;
    
    result = transition_result::success;
    return state_snapshot<app_state>(new_state, current.version() + 1);
}

// Replay events
state_snapshot<app_state> initial(app_state{0}, 0);
event_stream_result result;
state_snapshot<app_state> final = stream.replay(initial, transition_fn, result);

// Multiple replays produce IDENTICAL results
state_snapshot<app_state> final2 = stream.replay(initial, transition_fn, result);
// final.data() == final2.data() is guaranteed
```

### Explicit Event Processing

No callbacks - iterate over events explicitly:

```cpp
// Manual iteration
for (size_t i = 0; i < stream.count(); ++i) {
    const state_event<my_event>* event = stream.get(i);
    if (event != nullptr) {
        // Process event
    }
}

// Or use for_each helper
stream.for_each([](const state_event<my_event>& event) {
    // Process event
    return true; // Continue iteration
});
```

## Design Principles

### Determinism

Event stream guarantees deterministic behavior:

- **Ordered storage**: Events stored by sequence number
- **Ordered replay**: Events always applied in sequence order
- **No hidden state**: All state is explicit
- **Pure functions**: Transition functions have no side effects

Same input event stream + same initial state → **byte-identical final state**

### No Callbacks

Event processing is explicit:

- **No observers**: No callback registration
- **No implicit dispatch**: Events don't trigger automatic handlers
- **Explicit control**: Application controls when and how events are processed

This eliminates non-determinism from callback order and makes event flow explicit.

### Bounded Memory

Event streams have fixed capacity:

- **Configurable maximum**: Set `max_events` in config
- **Stream full detection**: Returns `event_stream_result::stream_full` when full
- **Explicit memory**: All memory usage is explicit and trackable

No unbounded growth prevents memory exhaustion.

## Integration with Aegis Architecture

### Frame Lifecycle Integration

Event streams integrate with the frame lifecycle's `apply_events` phase:

```cpp
#include "core/frame/frame_lifecycle.hpp"
#include "core/state/event_stream.hpp"

class my_executor : public frame_executor {
  protected:
    void on_apply_events(frame_context& ctx) noexcept override {
        // Replay events to update state
        event_stream_result result;
        next_state = event_stream_.replay(current_state, transition_fn, result);
        
        if (result == event_stream_result::success) {
            current_state = next_state;
        }
        
        // Clear stream for next frame
        event_stream_.clear();
    }
    
  private:
    event_stream<game_event> event_stream_;
    state_snapshot<game_state> current_state;
};
```

### State Transition Integration

Works seamlessly with state transition module:

```cpp
#include "core/state/state_transition.hpp"
#include "core/state/event_stream.hpp"

// Create transition
state_transition<app_state, app_event> transition(handle_event);

// Replay through transition
event_stream_result result;
auto final_state = stream.replay(initial_state, transition, result);
```

## Error Handling

All operations return result codes (no exceptions):

```cpp
event_stream_result result = stream.push(event_data, timestamp);

switch (result) {
    case event_stream_result::success:
        // Event added successfully
        break;
    case event_stream_result::stream_full:
        // Stream at capacity
        break;
    case event_stream_result::invalid_event:
        // Event validation failed
        break;
    case event_stream_result::sequence_error:
        // Sequence number conflict
        break;
    // Handle other cases...
}
```

## Performance Characteristics

- **Event push**: O(1) - constant time insertion
- **Event replay**: O(n) - linear in number of events
- **Event get**: O(1) - constant time lookup
- **Memory usage**: O(n) - linear in number of events (bounded by max_events)

## Statistics and Monitoring

Event streams track statistics:

```cpp
const event_stream_stats& stats = stream.stats();

printf("Total events: %zu\n", stats.total_events);
printf("Current events: %zu\n", stats.current_events);
printf("Events replayed: %zu\n", stats.events_replayed);
printf("Sequence range: [%lu, %lu]\n", stats.min_sequence, stats.max_sequence);
printf("Memory used: %zu bytes\n", stats.bytes_allocated);
```

## Example: Counter Application

See [demo_event_stream.cpp](demo_event_stream.cpp) for a complete example demonstrating:

- Event stream creation and configuration
- Adding events with automatic sequencing
- Deterministic replay (multiple replays produce identical results)
- Manual event iteration
- Integration with state transitions

Run the demo:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/core/event_stream_demo
```

## Acceptance Criteria Validation

### ✅ Events processed in deterministic order

```cpp
// Events are stored by sequence number
event_stream<E> stream;
stream.push(event1, 1000);  // sequence 0
stream.push(event2, 2000);  // sequence 1
stream.push(event3, 3000);  // sequence 2

// Replay always processes in order: event1, event2, event3
```

### ✅ Replaying event stream produces identical state

```cpp
// First replay
auto state1 = stream.replay(initial, transition_fn, result);

// Second replay
auto state2 = stream.replay(initial, transition_fn, result);

// Guaranteed: state1 == state2 (byte-identical)
```

### ✅ No callbacks or implicit dispatch

```cpp
// No callback registration - explicit iteration only
stream.for_each([](const auto& event) {
    // Explicitly called by application
    return true;
});

// Or explicit replay
auto final = stream.replay(initial, transition_fn, result);

// Events never trigger automatically
```

### ✅ No format and lint issues

```cpp
// Code formatted with clang-format
// Code analyzed with clang-tidy
// Builds without warnings (-Werror)
// Follows Aegis coding standards
```

## Architecture Compliance

This module adheres to the core folder contract:

- ✅ No OS dependencies
- ✅ No runtime dependencies  
- ✅ No hidden allocations (bounded fixed storage)
- ✅ Deterministic behavior
- ✅ Explicit memory management
- ✅ No RTTI or exceptions
- ✅ Follows coding standards (clang-format, clang-tidy)
