# Runtime Platform Adapter

## Overview

The `runtime/platform` module provides a minimal OS abstraction layer that maps platform-specific input to deterministic `core/events` types. This is the **only** place in the codebase where OS APIs are allowed.

## Architecture Contract

### Acceptance Criteria

✅ **OS input mapped to core/events**: Platform-specific events are converted to `input_event` types  
✅ **No OS APIs leak into core/**: All OS interaction is isolated to `runtime/`  
✅ **Deterministic behavior under replay**: Events can be recorded and replayed exactly  

### Architectural Boundaries

```
┌─────────────────────────────────────┐
│       Application Code              │
├─────────────────────────────────────┤
│       core/events                   │  ← OS-agnostic events
│       (Deterministic)                │
├─────────────────────────────────────┤
│       runtime/platform              │  ← Platform adapter (THIS LAYER)
│       (OS Abstraction)               │
├─────────────────────────────────────┤
│       OS APIs                        │  ← Linux, Windows, macOS
│       (Platform-Specific)            │
└─────────────────────────────────────┘
```

**Rule**: Only `runtime/` can depend on OS headers. `core/` must remain OS-agnostic.

## Key Components

### platform_adapter (Interface)

Abstract base class defining the platform abstraction contract:

- `initialize()` - Initialize platform adapter
- `shutdown()` - Clean up platform resources
- `poll_events()` - Poll OS events and convert to `core/events`
- `set_event_callback()` - Register event handler
- `get_time_ns()` - Get monotonic time for event timestamping

### null_platform_adapter (Implementation)

Headless platform adapter for testing and deterministic replay:

- No OS dependencies
- Manual event injection for testing
- Deterministic time control
- Ideal for unit tests and CI

### event_replay_buffer

Event recording and replay system:

- Records `input_event` sequences
- Deterministic replay through callback
- Bounded capacity for predictable memory usage

## Usage Example

```cpp
#include "runtime/platform/null_platform.hpp"
#include "core/events/input_event.hpp"

using namespace aegis::runtime::platform;
using namespace aegis::core::events;

// Event handler
bool handle_event(const input_event& event, void* user_data) {
    // Process event
    return true;
}

int main() {
    // Create platform adapter
    null_platform_adapter platform;
    platform.initialize();
    
    // Register callback
    platform.set_event_callback(handle_event, nullptr);
    
    // Inject events (for testing)
    auto event = input_event::make_keyboard(
        key_code::a,
        key_modifiers{false, false, false, false, 0},
        platform.get_time_ns()
    );
    platform.inject_event(event);
    
    // Poll events (normally called in game loop)
    platform.poll_events();
    
    platform.shutdown();
}
```

## Platform Implementations

### Current

- **null_platform**: Headless adapter for testing

### Planned

- **linux_platform**: X11/Wayland input integration
- **windows_platform**: Win32 input integration
- **macos_platform**: Cocoa input integration

Each platform implementation:
1. Polls OS-specific events
2. Converts to `core/events::input_event`
3. Timestamps with monotonic clock
4. Dispatches through callback

## Event Replay

Events can be recorded and replayed for deterministic testing:

```cpp
event_replay_buffer buffer(1024);

// Record events
buffer.push(event1);
buffer.push(event2);

// Replay events
buffer.replay_all(handle_event, nullptr);
```

This enables:
- Bug reproduction
- Performance profiling
- Automated testing
- Deterministic simulation

## Testing

See `demo_platform.cpp` for a complete example demonstrating:
- Platform initialization
- Event injection
- Event recording
- Event replay
- Platform shutdown

## Future Enhancements

- Platform-specific window creation
- Multi-monitor support
- Gamepad/joystick input
- IME (Input Method Editor) support
- Clipboard integration
