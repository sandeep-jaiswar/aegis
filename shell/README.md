# Aegis Shell (Viewer-Runtime)

## Overview

The Aegis shell is a **minimal viewer-runtime** that acts as SDL for Aegis, not a browser.

Think of it as:
- **SDL for Aegis** ✅
- **Chrome** ❌

## Design Principles

### Zero Logic in Shell

The shell contains **NO** business logic, rendering logic, or application logic.

All logic lives in `core/` and `runtime/`. The shell is just a thin bootstrapper.

### Determinism by Replay

**Kill shell, replay logs → identical output**

The shell:
1. Records all input events
2. Passes them to core/runtime
3. Can be killed and replaced
4. Replaying the same events produces identical behavior

### Language-Agnostic

**Shell can be rewritten in another language without touching core/**

The shell is:
- Not C++ by requirement
- Just an adapter/bootstrapper
- Replaceable with Python, Rust, Go, etc.
- Communicates via well-defined interfaces

## Scope

### What Shell Does

1. **Window Creation**
   - Create OS window
   - Handle window lifecycle (minimize, maximize, close)
   - Forward window events to runtime/platform

2. **Input → Event Mapping**
   - Capture OS input (keyboard, mouse, touch)
   - Map to `core/events::input_event`
   - Forward to runtime/platform

3. **GPU Surface**
   - Create GPU-compatible surface
   - Provide surface handle to runtime
   - Handle surface resize

4. **Module Loading**
   - Load `.aegis` modules
   - Verify module integrity
   - Initialize core/runtime with module

### What Shell Does NOT Do

- ❌ No rendering logic
- ❌ No event processing logic
- ❌ No state management
- ❌ No layout computation
- ❌ No GPU command generation
- ❌ No diff computation
- ❌ No memory management beyond basic initialization

## Architecture

```
┌─────────────────────────────────────┐
│          Shell (This Layer)         │
│   - Window creation                 │
│   - Input capture                   │
│   - GPU surface setup               │
│   - Module bootstrapping            │
├─────────────────────────────────────┤
│       runtime/platform              │
│   - Input event mapping             │
│   - Platform abstraction            │
├─────────────────────────────────────┤
│            core/                    │
│   - ALL logic lives here            │
└─────────────────────────────────────┘
```

## Usage

### Basic Shell Invocation

```bash
# Load and run a module
./aegis_shell app.aegis

# Run with event recording
./aegis_shell app.aegis --record events.log

# Replay recorded events
./aegis_shell app.aegis --replay events.log
```

### Event Replay Example

```bash
# Run 1: Record events
./aegis_shell trading_app.aegis --record session1.log

# Run 2: Replay exact same session
./aegis_shell trading_app.aegis --replay session1.log

# Outputs should be IDENTICAL
```

## Implementation Notes

### Current Implementation

The shell is implemented as a minimal C++ program that:
- Uses `runtime/platform` for OS abstraction
- Uses `core/module` for module loading
- Uses `core/events` for event types
- Provides a simple main loop

### Future Implementations

The shell could be rewritten in:
- **Python**: `python aegis_shell.py app.aegis`
- **Rust**: `cargo run --bin aegis_shell app.aegis`
- **Go**: `go run shell/main.go app.aegis`
- **JavaScript**: `node shell.js app.aegis`

As long as it:
1. Creates a window
2. Maps input to `core/events`
3. Provides a GPU surface
4. Loads modules via `core/module`

## Relationship to Other Components

### Shell vs Runtime

- **Shell**: Bootstraps the system, provides OS window
- **Runtime**: Abstracts platform differences, translates events

### Shell vs Core

- **Shell**: Minimal glue code, no logic
- **Core**: All application logic, deterministic

### Shell vs Platform

- **Shell**: High-level window creation
- **Platform**: Low-level OS abstraction

## Acceptance Criteria

✅ **Zero logic in shell**
- Shell has no conditional logic beyond argument parsing
- No computation, no state, no decision-making

✅ **Kill shell, replay logs → identical output**
- Events can be recorded and replayed
- Output is deterministic across runs
- Shell is stateless

✅ **Shell can be rewritten without touching core/**
- Well-defined interface boundaries
- Core is completely isolated
- Shell is just a thin adapter

## Conclusion

The shell is intentionally minimal. It exists only to bootstrap the Aegis runtime and provide OS integration.

**All intelligence lives in `core/` and `runtime/`.**

The shell is dumb by design.
