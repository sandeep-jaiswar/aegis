# Aegis Architecture

> **This document describes the technical architecture of Aegis.
> It exists to prevent accidental complexity, scope creep, and architectural drift.**

Aegis is designed as a **deterministic, C++-native, GPU-first application runtime** for performance-critical systems.

---

## 1. Architectural Goals (In Order of Priority)

1. **Deterministic performance** (P99 matters more than average)
2. **Explicit control over memory and execution**
3. **Linear, predictable scaling under load**
4. **Security by construction**
5. **Minimal, opinionated surface area**

If a design improves ergonomics but weakens any of the above, it is rejected.

---

## 2. High-Level System Overview

```text
┌──────────────────────────────────────────┐
│              Application Code            │
│           (C++ / optional WASM)           │
├──────────────────────────────────────────┤
│               State Engine               │
│     (Immutable state + event sourcing)   │
├──────────────────────────────────────────┤
│                Diff Engine               │
│        (SceneGraph structural diffs)     │
├──────────────────────────────────────────┤
│              Scene Graph                 │
│      (Retained-mode, GPU-friendly)       │
├──────────────────────────────────────────┤
│             Rendering Engine             │
│        (WebGPU / Vulkan backend)         │
├──────────────────────────────────────────┤
│          Platform & OS Abstraction       │
└──────────────────────────────────────────┘
```

There is **no DOM**, **no JavaScript runtime**, and **no CSS engine**.

---

## 3. Core Layer Responsibilities

### 3.1 Platform Layer (`platform/`)

**Responsibilities:**

* Window creation
* Input handling
* Timing and clocks
* Threading primitives
* OS-specific integration

**Constraints:**

* No allocation-heavy logic
* No business logic
* No rendering logic

> This layer exists only to abstract OS differences.

---

### 3.2 Rendering Engine (`render/`)

**Responsibilities:**

* GPU device management
* Command buffer generation
* Draw call batching
* Resource lifetime management

**Design Rules:**

* Retained-mode rendering
* Explicit resource ownership
* Minimal CPU↔GPU synchronization

**Non-Goals:**

* Immediate-mode APIs
* HTML/CSS rendering

---

### 3.3 Scene Graph (`scene/`)

**Responsibilities:**

* Persistent UI representation
* Spatial hierarchy
* Stable node identifiers

**Design Rules:**

* POD-friendly data structures
* No per-frame allocations
* GPU-compatible layouts

Scene nodes represent *what should be rendered*, not *how it is computed*.

---

### 3.4 Diff Engine (`diff/`)

**Responsibilities:**

* Compare previous and next scene graphs
* Generate minimal GPU command deltas

**Design Rules:**

* Structural diffs only
* No heuristic reconciliation
* Deterministic output

If two states are equal, diffs must be identical across runs.

---

### 3.5 State Engine (`state/`)

**Responsibilities:**

* Immutable application state
* Event application
* Snapshot generation
* Replay and audit support

**Design Rules:**

* No in-place mutation
* Event-sourced updates
* Clear ownership of state transitions

This layer is the **single source of truth**.

---

### 3.6 Application Layer (`app/`)

**Responsibilities:**

* Domain-specific logic
* UI composition
* Event handling

**Constraints:**

* No direct GPU access
* No hidden global state
* No ad-hoc threading

Applications describe **intent**, not execution.

---

## 4. Frame Lifecycle (Canonical)

Every frame follows this exact sequence:

```text
1. Input collection
2. Event dispatch
3. State transition
4. Scene graph construction
5. Diff computation
6. GPU command generation
7. Command submission
```

Rules:

* No stage may skip ahead
* No hidden work outside the frame loop
* All allocations must be attributable to a stage

---

## 5. Memory Model

### Principles

* No garbage collection
* No shared ownership in hot paths
* Bounded allocations per frame

### Allocator Types

* Frame allocator (reset every frame)
* Arena allocator (scene/state lifetime)
* Pool allocator (frequently reused objects)

Memory lifetimes must be obvious from scope.

---

## 6. Concurrency Model

* Single-threaded UI logic by default
* Background threads allowed only for:

  * IO
  * Data ingestion
  * Pre-processing

Synchronization rules:

* No blocking on the render thread
* Message passing over shared state

Determinism is preferred over maximum parallelism.

---

## 7. Security Model

* Core runtime is trusted
* Extensions run in sandboxed WASM
* Capability-based APIs
* No dynamic code loading in core

Security boundaries must be architectural, not conventional.

---

## 8. What We Explicitly Do NOT Build

* HTML / CSS engines
* JavaScript runtimes
* Browser compatibility layers
* Ad-hoc plugin systems
* Implicit reactivity systems

If a feature resembles browser behavior, it is scrutinized heavily.

---

## 9. Evolution Rules

Changes must:

* Preserve determinism
* Be measurable
* Be removable

If a feature cannot be benchmarked or justified with data, it does not ship.

---

## 10. Final Constraint

> **Aegis is an engine, not a framework.**

It prioritizes correctness, performance, and predictability over flexibility and familiarity.
