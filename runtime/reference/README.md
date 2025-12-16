# Aegis Reference Runtime Viewer

**Ticket:** REF-001  
**Status:** Complete  
**Version:** 1.0.0

---

## Overview

The Reference Runtime Viewer defines the **minimal interface** that any Aegis runtime shell must implement. This specification ensures that runtime implementations can be swapped without modifying `core/`.

Think of this as:
- **The canonical "what is a runtime" specification** ✅
- **NOT an implementation** ❌

---

## Purpose

### REF-001: Minimal Reference Viewer

**Goals:**
1. Define the smallest possible runtime shell interface
2. Enable deterministic replay of recorded sessions
3. Allow runtime replacement without touching `core/`

**Scope:**
- Window creation
- Input → event mapping
- GPU surface setup
- Module loader
- Event recording/replay

---

## Interface Definition

The `reference_viewer_interface` class defines the contract for all runtime viewers:

```cpp
class reference_viewer_interface {
public:
    // Initialization
    virtual bool initialize() noexcept = 0;
    virtual void shutdown() noexcept = 0;
    
    // Window creation (REF-001)
    virtual bool create_window(const window_config&) noexcept = 0;
    virtual void destroy_window() noexcept = 0;
    virtual bool should_close() const noexcept = 0;
    
    // Input → event mapping (REF-001)
    virtual void set_event_callback(event_callback_fn, void*) noexcept = 0;
    virtual size_t poll_events() noexcept = 0;
    
    // GPU surface setup (REF-001)
    virtual bool create_gpu_surface(const gpu_surface_config&) noexcept = 0;
    virtual void* get_gpu_surface_handle() const noexcept = 0;
    virtual void destroy_gpu_surface() noexcept = 0;
    
    // Module loader (REF-001)
    virtual module_load_result load_module(const module_load_config&) noexcept = 0;
    virtual void unload_module() noexcept = 0;
    virtual const module_header* get_module_header() const noexcept = 0;
    
    // Timing (deterministic replay)
    virtual uint64_t get_time_ns() const noexcept = 0;
    
    // Recording & replay (REF-001 acceptance criterion)
    virtual bool start_recording(const char*) noexcept = 0;
    virtual void stop_recording() noexcept = 0;
    virtual bool start_replay(const char*) noexcept = 0;
    virtual void stop_replay() noexcept = 0;
    virtual bool is_recording() const noexcept = 0;
    virtual bool is_replaying() const noexcept = 0;
};
```

---

## Acceptance Criteria

### ✅ Zero Business Logic in Shell

The interface defines **only** OS integration points:
- Window creation (OS API)
- Input mapping (OS events → `core::events`)
- GPU surface (OS graphics API)
- Module loading (file I/O)

**No logic for:**
- ❌ Rendering
- ❌ Layout computation
- ❌ State management
- ❌ Event processing (beyond mapping)
- ❌ Scene graph construction

All logic lives in `core/`.

### ✅ Killing Viewer + Replaying Log → Identical Output

**Recording:**
```cpp
viewer->start_recording("session.log");
// Run application...
viewer->stop_recording();
```

**Replay:**
```cpp
viewer->start_replay("session.log");
// Replay application with same inputs...
viewer->stop_replay();
```

**Guarantee:** Same event log → same `input_event` sequence → same `core/` behavior

**Why this works:**
1. `core/` is deterministic (see [DETERMINISM.md](../../docs/DETERMINISM.md))
2. Event recording captures all inputs
3. Replay uses recorded events instead of OS events
4. No external state → deterministic execution

### ✅ Shell Can Be Replaced Without Touching core/

The interface is **implementation-agnostic**:

**C++ Implementation:**
```cpp
class sdl_viewer : public reference_viewer_interface {
    // Uses SDL2 for window/input
};

class glfw_viewer : public reference_viewer_interface {
    // Uses GLFW for window/input
};
```

**Python Implementation:**
```python
class PythonViewer(ReferenceViewerInterface):
    # Uses PyGame or PyQt
```

**Rust Implementation:**
```rust
struct WinitViewer;
impl ReferenceViewerInterface for WinitViewer {
    // Uses winit for window/input
}
```

**Key Point:** `core/` depends on `core::events::input_event`, NOT on viewer implementation.

---

## Design Principles

### 1. Minimal Surface Area

The interface includes **only** what's necessary for runtime integration:
- Window (OS interaction)
- Input mapping (OS → `core::events`)
- GPU surface (graphics API)
- Module loading (file I/O)
- Recording/replay (determinism)

**No optional features. No convenience APIs.**

### 2. Deterministic Replay

Every method that interacts with external state (OS, filesystem) must support deterministic replay:

| Method | Replay Behavior |
|--------|----------------|
| `poll_events()` | Replays recorded events instead of OS events |
| `get_time_ns()` | Returns recorded timestamps during replay |
| `load_module()` | Loads same module deterministically |
| `create_window()` | Window config from recorded session |
| `create_gpu_surface()` | Surface config from recorded session |

### 3. Zero Business Logic

The viewer is **dumb by design**:
- No rendering logic (core/ generates GPU commands)
- No layout logic (core/ computes layouts)
- No state logic (core/ manages state)
- No event processing (core/ processes events)

The viewer **only** maps OS primitives to `core/` primitives.

### 4. Language Agnostic

The interface can be implemented in any language:
- C++ (reference implementation)
- Python (for rapid prototyping)
- Rust (for safety-critical systems)
- Go (for network services)
- JavaScript/TypeScript (for web embedding)

**Requirement:** Must call `core/` APIs correctly and maintain determinism.

---

## Implementation Guidelines

### Conformance Checklist

A conforming runtime viewer implementation MUST:

- [ ] Implement all `reference_viewer_interface` methods
- [ ] Map OS input to `core::events::input_event` deterministically
- [ ] Support event recording to file
- [ ] Support event replay from file with byte-identical results
- [ ] Create GPU-compatible surface (WebGPU, Vulkan, or compatible)
- [ ] Load `.aegis` modules using `core::module::module_loader`
- [ ] Verify module capabilities against runtime capabilities
- [ ] Provide monotonic timestamp in nanoseconds
- [ ] Clean up all resources in `shutdown()`

### Testing

Conforming implementations should pass the **Runtime Conformance Test Suite** (REF-002).

See [runtime/conformance_tests/](../conformance_tests/) for test specifications.

---

## Example Implementations

### Null Viewer (Headless Testing)

```cpp
#include "reference_viewer.hpp"
#include "../platform/null_platform.hpp"

class null_reference_viewer : public reference_viewer_interface {
    null_platform_adapter platform_;
    // Minimal headless implementation for testing
};
```

### SDL Viewer (Desktop GUI)

```cpp
#include "reference_viewer.hpp"
#include <SDL2/SDL.h>

class sdl_reference_viewer : public reference_viewer_interface {
    SDL_Window* window_;
    // Uses SDL2 for real window/input
};
```

### Web Viewer (Browser Embedding)

```javascript
class WebReferenceViewer {
    // Implements reference_viewer_interface via WebAssembly
    // Uses Canvas API for window
    // Uses DOM events for input
}
```

---

## Relationship to Other Components

### Reference Viewer vs Shell

- **Reference Viewer**: Abstract interface specification (this document)
- **Shell**: Concrete implementation using reference viewer interface

The shell (`shell/main.cpp`) is a minimal executable that:
1. Instantiates a concrete viewer (e.g., `null_reference_viewer`)
2. Parses command-line arguments
3. Calls viewer methods
4. Runs main loop

### Reference Viewer vs Runtime

- **Reference Viewer**: Minimal OS integration layer
- **Runtime**: Full platform abstraction layer (`runtime/platform/`)

The runtime provides lower-level platform abstractions. The reference viewer uses these abstractions to implement the viewer interface.

### Reference Viewer vs Core

- **Reference Viewer**: Provides inputs to core (events, timestamps, modules)
- **Core**: Processes inputs deterministically, generates outputs

The viewer has **no dependency** on core internals. It only depends on:
- `core::events::input_event` (event type)
- `core::module::module_loader` (module loading)
- `core::module::module_header` (module metadata)

---

## Usage Example

```cpp
#include "runtime/reference/reference_viewer.hpp"
#include "runtime/platform/null_platform.hpp"

using namespace aegis::runtime::reference;

int main(int argc, char** argv) {
    // Create viewer (implementation-specific)
    auto viewer = create_null_viewer();
    
    // Initialize
    if (!viewer->initialize()) {
        return 1;
    }
    
    // Create window
    window_config win_cfg{
        .title = "Aegis App",
        .width = 1920,
        .height = 1080
    };
    if (!viewer->create_window(win_cfg)) {
        return 1;
    }
    
    // Create GPU surface
    gpu_surface_config gpu_cfg{
        .width = 1920,
        .height = 1080,
        .vsync = true
    };
    if (!viewer->create_gpu_surface(gpu_cfg)) {
        return 1;
    }
    
    // Load module
    module_load_config mod_cfg{
        .module_path = "app.aegis",
        .runtime_capabilities = 0x0001 // gpu_rendering
    };
    auto result = viewer->load_module(mod_cfg);
    if (result != module_load_result::success) {
        return 1;
    }
    
    // Set event callback
    viewer->set_event_callback([](const auto& event, void*) {
        // Process event in core/
        return true;
    }, nullptr);
    
    // Main loop
    while (!viewer->should_close()) {
        viewer->poll_events();
        // core/ processes events, generates GPU commands
        // viewer submits GPU commands (not in interface)
    }
    
    // Cleanup
    viewer->unload_module();
    viewer->destroy_gpu_surface();
    viewer->destroy_window();
    viewer->shutdown();
    
    return 0;
}
```

---

## Future Enhancements

While the current interface is frozen for v1.0, future versions may add:

1. **Multi-window support** (v1.1)
2. **Multi-monitor configuration** (v1.1)
3. **VR/AR device integration** (v2.0)
4. **Remote rendering support** (v2.0)
5. **Cloud gaming backend** (v3.0)

All additions must maintain determinism and backward compatibility.

---

## Conclusion

The Reference Runtime Viewer specification defines the **absolute minimum** interface for an Aegis runtime shell.

**Key Properties:**
- ✅ Zero business logic
- ✅ Deterministic replay
- ✅ Language agnostic
- ✅ Implementation independent
- ✅ Core-compatible

This specification enables clean-room runtime implementations that can be swapped without modifying `core/`.

**Think SDL for Aegis, not Chrome.**

---

**End of Reference Runtime Viewer Specification**
