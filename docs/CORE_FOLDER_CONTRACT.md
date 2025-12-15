# Architecture: Core Folder Contract (Non‑Negotiable)

This document defines the **hard dependency boundaries** of the Aegis runtime.

These rules exist to:

* Preserve determinism
* Prevent architectural erosion
* Keep performance explainable

If a change violates this contract, it is **incorrect by definition**, regardless of functionality.

---

## Mental Model

> **Core is the engine. Everything else is replaceable.**

`core/` must be:

* Runtime-agnostic
* UI-agnostic
* Platform-agnostic
* Application-agnostic

It is where correctness and performance are enforced.

---

## Top-Level Repository Structure

```
/aegis
 ├── core/          # Deterministic engine (this document)
 ├── runtime/       # OS, windowing, GPU, IO glue
 ├── app/           # Concrete applications (order book, grid)
 ├── tools/         # Profilers, benchmarks, replay tools
 ├── docs/          # Architecture & design docs
 └── tests/         # Deterministic tests
```

**Only `runtime/` is allowed to talk to the OS.**

---

## The Core Rule (Memorize This)

> **Nothing in `core/` may depend on anything outside `core/`.**

This includes:

* OS APIs
* Windowing systems
* GPU APIs
* Networking
* Filesystems
* Threads
* Clocks

Core operates on **pure data and explicit inputs** only.

---

## `core/` Submodules and Contracts

### 1️⃣ `core/memory/`

**Purpose:** Deterministic memory management

Allowed:

* Arena allocators
* Frame allocators
* Pools
* Stack-based lifetimes

Banned:

* `new` / `delete` in hot paths
* Global allocators
* Implicit heap usage

Rules:

* Allocation cost must be explainable
* Lifetimes must be obvious from scope

---

### 2️⃣ `core/state/`

**Purpose:** Immutable application state

Allowed:

* Value types
* Copy-on-write if explicit
* Structural sharing (manual)

Banned:

* Mutable global state
* Hidden caches

Rules:

* State transitions must be pure
* Previous state must remain valid

---

### 3️⃣ `core/events/`

**Purpose:** Deterministic input and system events

Allowed:

* Typed events
* Explicit ordering
* Timestamped inputs

Banned:

* Callbacks
* Implicit bubbling
* Thread-local event sources

Rules:

* Events are data, not behavior

---

### 4️⃣ `core/layout/`

**Purpose:** Deterministic layout computation

Allowed:

* One-pass layout
* Explicit constraints
* Cacheable results

Banned:

* Cascading rules
* Contextual dependencies
* Style inheritance

Rules:

* Same input → same output

---

### 5️⃣ `core/scene/`

**Purpose:** Retained-mode scene graph

Allowed:

* Stable node IDs
* Flat or shallow trees
* Explicit parent-child relations

Banned:

* Implicit ordering
* Self-mutating nodes

Rules:

* Scene graph is immutable per frame

---

### 6️⃣ `core/diff/`

**Purpose:** Structural diffs between frames

Allowed:

* Scene-to-scene comparison
* Explicit change sets

Banned:

* Heuristic diffing
* Tree walking without bounds

Rules:

* Diff cost proportional to change

---

### 7️⃣ `core/math/`

**Purpose:** Fast, predictable math

Allowed:

* SIMD-friendly structures
* Fixed-point where needed

Banned:

* Implicit floating-point nondeterminism

Rules:

* Numerical stability is mandatory

---

## What `core/` MUST NOT Know

`core/` must never know about:

* Windows / Linux / macOS
* Vulkan / Metal / DX12
* Mouse, keyboard, touch APIs
* Network protocols
* Files

Those belong in `runtime/`.

---

## `runtime/` Contract (Brief)

`runtime/`:

* Translates OS input → `core/events`
* Translates `core/scene` diffs → GPU commands
* Owns threads, clocks, IO

`runtime/` may depend on `core/`.

`core/` may never depend on `runtime/`.

---

## Application Layer (`app/`)

`app/`:

* Defines domain state (orders, trades, grids)
* Builds scene graphs from state
* Contains no rendering logic

If logic is reusable or performance-critical, it moves down into `core/`.

---

## Tests & Verification

### Core Tests Must Be:

* Deterministic
* Replayable
* Platform-independent

If a test requires an OS, it does not belong to `core/`.

---

## Enforcement Rules

* Violations block merges
* Convenience is not a justification
* Performance regressions require explanation

---

## Design Smell Checklist

If you see any of these, stop:

* "Just this once"
* "We can optimize later"
* "The OS guarantees this"
* "It probably won’t matter"

They always matter.

---

## Final Rule

> **Core exists to make the system explainable.**

If a behavior cannot be explained from `core/` alone, the architecture has failed.
