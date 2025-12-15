# Core Events Module

## Overview

The `core/events` module provides deterministic, platform-agnostic input event types for the Aegis runtime. These events are designed to be replayed exactly, enabling deterministic behavior for debugging and testing.

## Architecture

### Design Principles

1. **OS-Agnostic**: No OS-specific types or dependencies
2. **Deterministic**: Events can be replayed to produce identical behavior
3. **Compact**: Events are cache-friendly and suitable for high-frequency input
4. **Type-Safe**: Strongly typed events with compile-time guarantees

### Event Types

All input events are represented as `input_event` structs with the following types:

- **Keyboard Events**: `keyboard_down`, `keyboard_up`
- **Mouse Events**: `mouse_move`, `mouse_button_down`, `mouse_button_up`, `mouse_scroll`
- **Touch Events**: `touch_begin`, `touch_move`, `touch_end`, `touch_cancel`

## Key Components

### input_event

The main event structure containing:
- Event type discriminator
- Timestamp (nanoseconds)
- Event-specific data (keyboard, mouse, scroll, or touch)

### Factory Functions

Static factory functions for creating events:
- `make_keyboard()` - Create keyboard event
- `make_mouse_move()` - Create mouse movement event
- `make_mouse_button()` - Create mouse button event
- `make_scroll()` - Create scroll event
- `make_touch()` - Create touch event

## Usage Example

```cpp
#include "core/events/input_event.hpp"

using namespace aegis::core::events;

// Create keyboard event
auto kbd_event = input_event::make_keyboard(
    key_code::a, 
    key_modifiers{false, false, false, false, 0},
    1000000 // timestamp in nanoseconds
);

// Create mouse event
auto mouse_event = input_event::make_mouse_move(
    100.0F, 200.0F,  // x, y position
    5.0F, 10.0F,     // dx, dy delta
    key_modifiers{false, false, false, false, 0},
    2000000
);
```

## Integration with Runtime

The `runtime/platform` layer is responsible for:
1. Capturing OS-specific input events
2. Converting them to `core/events` types
3. Timestamping events with monotonic clock
4. Dispatching to the core engine

This ensures that **no OS APIs leak into `core/`**, maintaining architectural boundaries.

## Replay Support

Events can be recorded and replayed for:
- **Debugging**: Reproduce exact user interactions
- **Testing**: Automated input testing
- **Profiling**: Performance analysis with real input patterns

See `runtime/platform/platform_adapter.hpp` for the event replay buffer implementation.
