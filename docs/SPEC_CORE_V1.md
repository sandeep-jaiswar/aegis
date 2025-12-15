# Aegis Core Runtime Specification v1.0

**Status:** Frozen  
**Version:** 1.0.0  
**Last Updated:** 2025-12-15

---

## 1. Purpose and Scope

This document provides a complete, formal specification of the Aegis Core Runtime (`core/`) semantics. It defines the exact behavior required for a conforming implementation.

### 1.1 Specification Goals

1. **No implementation details**: All core behavior is explicitly specified
2. **Independent reimplementation**: A second engineer can reimplement `core/` from this spec alone
3. **Deterministic execution**: Same inputs produce byte-identical outputs
4. **Verifiable correctness**: All requirements are testable

### 1.2 Conformance

An implementation is conforming if and only if:

- It implements all MUST requirements in this specification
- It produces identical outputs for identical inputs (see [DETERMINISM.md](DETERMINISM.md))
- All acceptance criteria pass with a clean-room build

---

## 2. Core Architecture

### 2.1 Module Structure

The core runtime consists of the following modules, listed in dependency order (no circular dependencies):

```
core/
├── memory/         # Memory allocators (no dependencies)
├── events/         # Event types and structures (depends on: none)
├── state/          # Immutable state management (depends on: memory)
├── layout/         # Layout computation (depends on: memory)
├── frame/          # Frame lifecycle and scene graph (depends on: memory, events, state)
└── benchmark/      # Performance measurement (depends on: frame, memory)
```

### 2.2 Dependency Rules

**MUST NOT:**
- Core modules MUST NOT depend on any code outside `core/`
- Core modules MUST NOT use OS headers or APIs
- Core modules MUST NOT use threading primitives
- Core modules MUST NOT use system clocks or timers
- Core modules MUST NOT use file I/O
- Core modules MUST NOT use networking
- Core modules MUST NOT use dynamic linking

**MUST:**
- All external inputs MUST be passed explicitly via function parameters
- All timestamps MUST be provided by the caller
- All buffers MUST be provided by the caller
- All module dependencies MUST be acyclic

---

## 3. Frame Lifecycle Contract (Frozen)

### 3.1 Frame Execution Model

Every frame MUST execute exactly 7 phases in strict sequential order:

```
Frame N:
  1. begin_frame(timestamp_ns)
  2. apply_events()
  3. update_state()
  4. compute_layout()
  5. build_scene()
  6. diff_scene()
  7. end_frame()
```

### 3.2 Phase Definitions

#### 3.2.1 Phase 1: begin_frame(uint64_t timestamp_ns)

**Purpose:** Initialize frame state and timing.

**Preconditions:**
- Current phase MUST be `idle` or `end`
- `timestamp_ns` MUST be ≥ previous frame's timestamp

**Postconditions:**
- Current phase becomes `begin`
- `frame_number` increments by 1
- `frame_start_timestamp_ns` is set to `timestamp_ns`
- `phase_start_time_ns` is set to `timestamp_ns`
- All timing accumulators are reset to 0

**MUST:**
- Record the start timestamp exactly as provided
- Initialize all frame statistics to zero
- Transition phase state atomically

**MUST NOT:**
- Perform any allocations
- Access external state
- Perform I/O

#### 3.2.2 Phase 2: apply_events()

**Purpose:** Process input and system events.

**Preconditions:**
- Current phase MUST be `begin`

**Postconditions:**
- Current phase becomes `apply_events`
- All queued events are processed or deferred
- Event order is preserved

**MUST:**
- Process events in the order received
- Timestamp all events relative to frame start
- Maintain event determinism (see [DETERMINISM.md](DETERMINISM.md))

**MUST NOT:**
- Mutate application state (state updates occur in phase 3)
- Skip or reorder events
- Generate new events not from input

#### 3.2.3 Phase 3: update_state()

**Purpose:** Update application state based on events.

**Preconditions:**
- Current phase MUST be `apply_events`

**Postconditions:**
- Current phase becomes `update_state`
- Application state reflects all applied events
- Previous state remains immutable

**MUST:**
- Produce new immutable state
- Preserve previous state (immutable snapshots)
- Apply state transitions deterministically

**MUST NOT:**
- Mutate previous state in-place
- Access scene graph from previous frame
- Perform rendering operations

#### 3.2.4 Phase 4: compute_layout()

**Purpose:** Calculate UI element positions and sizes.

**Preconditions:**
- Current phase MUST be `update_state`

**Postconditions:**
- Current phase becomes `compute_layout`
- All layout constraints are resolved
- Layout results are deterministic

**MUST:**
- Resolve all constraints in one pass
- Produce deterministic layout (same inputs → same layout)
- Complete within bounded time

**MUST NOT:**
- Mutate application state
- Perform cascading style resolution
- Access GPU resources

#### 3.2.5 Phase 5: build_scene()

**Purpose:** Construct scene graph from application state and layout.

**Preconditions:**
- Current phase MUST be `compute_layout`

**Postconditions:**
- Current phase becomes `build_scene`
- Complete scene graph is constructed
- All nodes have stable identifiers

**MUST:**
- Assign stable node IDs (deterministic across identical builds)
- Maintain parent-child relationships explicitly
- Produce deterministic scene structure

**MUST NOT:**
- Mutate previous frame's scene graph
- Allocate GPU resources
- Perform rendering

#### 3.2.6 Phase 6: diff_scene()

**Purpose:** Compute structural differences between current and previous scene graphs.

**Preconditions:**
- Current phase MUST be `build_scene`

**Postconditions:**
- Current phase becomes `diff_scene`
- Minimal change set is computed
- Diff is deterministic

**MUST:**
- Produce byte-identical diffs for identical scene pairs
- Generate minimal change sets (no redundant operations)
- Complete in O(changes) time

**MUST NOT:**
- Use heuristic reconciliation
- Mutate either scene graph
- Generate GPU commands (runtime's responsibility)

#### 3.2.7 Phase 7: end_frame()

**Purpose:** Finalize frame execution and validate timing.

**Preconditions:**
- Current phase MUST be `diff_scene`

**Postconditions:**
- Current phase becomes `end`
- Total frame time is computed
- Memory statistics are captured
- Frame allocator is reset (if present)

**MUST:**
- Calculate exact total frame time
- Check against time budget
- Reset frame allocator to initial state
- Capture peak memory usage
- Return `exceeded_time_budget` if budget exceeded

**MUST NOT:**
- Allocate new memory
- Access GPU
- Start next frame

### 3.3 Phase Transition Validation

All phase transitions MUST be validated:

```cpp
bool is_valid_transition(current_phase, next_phase) {
    static const valid_transitions = {
        {idle,           begin},
        {end,            begin},
        {begin,          apply_events},
        {apply_events,   update_state},
        {update_state,   compute_layout},
        {compute_layout, build_scene},
        {build_scene,    diff_scene},
        {diff_scene,     end}
    };
    return valid_transitions.contains({current_phase, next_phase});
}
```

Invalid transitions MUST return `frame_result::invalid_phase_transition`.

### 3.4 Timing Requirements

**Time Budget Enforcement:**
- Default budget: 16,666,667 ns (60 FPS)
- Budget is configurable via `set_time_budget_ns()`
- Budget check occurs in `end_frame()`
- Returns `frame_result::exceeded_time_budget` if exceeded

**Phase Timing:**
- Each phase records start and end timestamps
- Phase time = end_timestamp - start_timestamp
- Total time = sum of all phase times
- All timing values MUST be deterministic for replay

---

## 4. Memory Management Contract (Frozen)

### 4.1 Allocator Types

Core provides three allocator types with precise semantics:

#### 4.1.1 Arena Allocator

**Purpose:** Bulk allocation with efficient reset.

**Characteristics:**
- **Allocation:** O(1) - bump pointer allocation
- **Deallocation:** O(1) - no-op (tracked for statistics only)
- **Reset:** O(1) - resets offset to zero
- **Alignment:** Configurable, must be power of 2
- **Overhead:** 0 bytes per allocation

**Interface Contract:**

```cpp
class arena_allocator {
public:
    // Constructor MUST NOT allocate
    // buffer MUST remain valid for allocator lifetime
    arena_allocator(uint8_t* buffer, size_t size) noexcept;
    
    // Allocate 'size' bytes with 'alignment'
    // Returns nullptr if insufficient space
    // MUST be O(1)
    void* allocate(size_t size, size_t alignment) noexcept;
    
    // Deallocation is tracked but does not free memory
    // MUST be O(1)
    void deallocate(void* ptr, size_t size) noexcept;
    
    // Reset to initial state
    // MUST be O(1)
    void reset() noexcept;
    
    // Statistics (MUST be exact)
    size_t bytes_allocated() const noexcept;
    size_t bytes_freed() const noexcept;
    size_t bytes_in_use() const noexcept;
    size_t peak_bytes_used() const noexcept;
};
```

**MUST:**
- Return correctly aligned pointers
- Track all allocations and deallocations exactly
- Reset to pristine state in O(1) time
- Return nullptr on allocation failure

**MUST NOT:**
- Perform heap allocations
- Free individual allocations
- Modify caller's buffer on construction

#### 4.1.2 Frame Allocator

**Purpose:** Per-frame linear allocator with automatic reset.

**Characteristics:**
- **Allocation:** O(1) - bump pointer allocation
- **Deallocation:** O(1) - no-op (tracked for statistics only)
- **Reset:** O(1) - automatic at end_frame()
- **Alignment:** Configurable, must be power of 2
- **Overhead:** 0 bytes per allocation

**Interface Contract:**

```cpp
class frame_allocator {
public:
    frame_allocator(uint8_t* buffer, size_t size) noexcept;
    
    void* allocate(size_t size, size_t alignment) noexcept;
    void deallocate(void* ptr, size_t size) noexcept;
    void reset() noexcept;
    
    // Additional tracking
    size_t get_allocation_count() const noexcept;
    size_t get_deallocation_count() const noexcept;
    
    size_t bytes_allocated() const noexcept;
    size_t bytes_freed() const noexcept;
    size_t bytes_in_use() const noexcept;
    size_t peak_bytes_used() const noexcept;
};
```

**Lifecycle Integration:**

The frame allocator MUST be integrated with `frame_context`:

1. At construction, `frame_context` accepts optional `frame_allocator*`
2. During frame execution, allocations are tracked
3. At `end_frame()`:
   - Statistics are captured: `frame_allocations`, `frame_deallocations`
   - Memory stats are updated: `bytes_allocated`, `bytes_freed`, `peak_memory_used`
   - Allocator is reset to initial state
4. Next frame starts with clean allocator

**MUST:**
- Reset automatically at end of every frame
- Track allocation/deallocation counts separately
- Integrate statistics into `frame_stats`

#### 4.1.3 Pool Allocator

**Purpose:** Fixed-size object allocation with free-list management.

**Characteristics:**
- **Allocation:** O(1) - pop from free list
- **Deallocation:** O(1) - push to free list
- **Reset:** O(n) where n = number of blocks
- **Alignment:** Configurable per block
- **Overhead:** Pointer-sized overhead per block in free list

**Interface Contract:**

```cpp
class pool_allocator {
public:
    // Initialize pool with block_size and block_alignment
    pool_allocator(uint8_t* buffer, size_t size,
                   size_t block_size, size_t block_alignment) noexcept;
    
    // Allocate one block (size/alignment parameters ignored)
    void* allocate(size_t size, size_t alignment) noexcept;
    
    // Return block to pool
    void deallocate(void* ptr, size_t size) noexcept;
    
    // Rebuild free list
    void reset() noexcept;
    
    size_t bytes_allocated() const noexcept;
    size_t bytes_freed() const noexcept;
    size_t bytes_in_use() const noexcept;
    size_t peak_bytes_used() const noexcept;
};
```

**MUST:**
- Maintain free list correctly
- Support allocation/deallocation in any order
- Return blocks to free list on deallocation
- Rebuild complete free list on reset

### 4.2 Memory Alignment

All allocators MUST support proper alignment:

```cpp
// Alignment calculation (MUST handle overflow)
size_t align_up(size_t value, size_t alignment) noexcept {
    if (alignment == 0 || value > (SIZE_MAX - alignment + 1)) {
        return value; // Overflow protection
    }
    return (value + alignment - 1) & ~(alignment - 1);
}
```

**MUST:**
- Alignment MUST be a power of 2
- Returned pointers MUST be aligned to requested alignment
- Handle overflow cases safely
- Default alignment: `alignof(std::max_align_t)`

### 4.3 Memory Tracking

All memory operations MUST be tracked:

```cpp
// In frame_context
void track_allocation(size_t bytes) noexcept {
    stats.bytes_allocated += bytes;
    size_t current = stats.bytes_allocated - stats.bytes_freed;
    if (current > stats.peak_memory_used) {
        stats.peak_memory_used = current;
    }
}

void track_deallocation(size_t bytes) noexcept {
    stats.bytes_freed += bytes;
}
```

**MUST:**
- Track every allocation precisely
- Track every deallocation precisely
- Calculate peak memory usage
- Expose statistics via `frame_stats`

### 4.4 Memory Safety Guarantees

**Buffer Lifetime:**
- Allocators MUST NOT own their buffers
- Buffers MUST remain valid for allocator lifetime
- Caller is responsible for buffer allocation/deallocation

**Allocation Failure:**
- Allocators MUST return `nullptr` on failure
- MUST NOT throw exceptions
- MUST NOT terminate program

**Buffer Overrun Protection:**
- Allocators MUST check bounds before allocation
- MUST NOT write beyond buffer end
- MUST NOT return overlapping regions

---

## 5. State Management Contract

### 5.1 Immutability Principle

**MUST:**
- All state transitions MUST produce new state
- Previous state MUST remain immutable
- State objects MUST NOT be mutated in-place

**MUST NOT:**
- Modify previous state
- Share mutable references
- Use hidden caches or memoization that affects determinism

### 5.2 State Snapshot

```cpp
class state_snapshot {
public:
    // Create initial state
    static state_snapshot create_initial() noexcept;
    
    // Apply event to produce new state
    state_snapshot apply_event(const event& evt) const noexcept;
    
    // State MUST be copyable and comparable
    state_snapshot(const state_snapshot&) noexcept = default;
    state_snapshot& operator=(const state_snapshot&) noexcept = default;
    
    // Equality MUST be byte-for-byte comparison
    bool operator==(const state_snapshot&) const noexcept;
};
```

### 5.3 State Transition Contract

**Determinism:**
- `state.apply_event(evt)` MUST produce identical result for identical inputs
- Event order MUST be preserved
- No hidden randomness or timing dependencies

**Event Sourcing:**
- All state changes MUST be event-driven
- Events MUST be immutable
- Event history enables replay and audit

---

## 6. Event System Contract

### 6.1 Event Types

Events MUST be POD (Plain Old Data) types:

```cpp
enum class input_event_type : uint8_t {
    none = 0,
    mouse_move = 1,
    mouse_button = 2,
    key_press = 3,
    key_release = 4,
    scroll = 5
};

struct input_event {
    input_event_type type;
    uint64_t timestamp_ns;  // Relative to frame start
    // ... type-specific data (POD only)
};
```

**MUST:**
- Be POD types (no vtables, no destructors)
- Include timestamp
- Be copyable and comparable
- Support deterministic serialization

### 6.2 Event Ordering

**MUST:**
- Process events in timestamp order
- Preserve causality (event A before event B)
- Break ties deterministically (e.g., by sequence number)

**MUST NOT:**
- Reorder events arbitrarily
- Drop events without explicit policy
- Merge events that should be separate

---

## 7. Layout Engine Contract

### 7.1 Layout Computation

**MUST:**
- Compute layout in one pass (no iteration)
- Produce deterministic results (same constraints → same layout)
- Complete in bounded time
- Support explicit constraints only

**MUST NOT:**
- Use cascading style inheritance
- Perform heuristic layout
- Access network or fonts dynamically

### 7.2 Constraint System

Constraints MUST be explicit and deterministic:

```cpp
struct layout_constraints {
    float min_width;
    float max_width;
    float min_height;
    float max_height;
    // ... explicit constraints only
};

struct layout_result {
    float x, y;
    float width, height;
};
```

---

## 8. Scene Graph Contract

### 8.1 Node Structure

Scene nodes MUST be immutable per frame:

```cpp
struct scene_node {
    uint64_t node_id;      // Stable identifier
    uint64_t parent_id;    // 0 for root
    float x, y;            // Position
    float width, height;   // Size
    // ... rendering properties (POD only)
};
```

**MUST:**
- Assign stable node IDs (deterministic)
- Maintain explicit parent-child relationships
- Be immutable after construction
- Support deterministic serialization

### 8.2 Scene Diff Contract

```cpp
enum class diff_op_type : uint8_t {
    add_node,
    remove_node,
    update_node,
    move_node
};

struct scene_diff_op {
    diff_op_type type;
    uint64_t node_id;
    // ... operation-specific data
};
```

**MUST:**
- Produce minimal diff (no redundant operations)
- Generate byte-identical diffs for identical scene pairs
- Complete in O(changes) time
- Support deterministic replay

---

## 9. Benchmark System Contract

### 9.1 Benchmark Interface

```cpp
class benchmark {
public:
    virtual const char* name() const noexcept = 0;
    virtual void setup() noexcept = 0;
    virtual void execute() noexcept = 0;
    virtual void teardown() noexcept = 0;
    virtual bool verify() const noexcept = 0;
    virtual uint64_t get_workload_hash() const noexcept = 0;
};
```

### 9.2 Deterministic Benchmarking

**MUST:**
- Support workload recording and replay
- Produce identical results for identical workloads
- Calculate percentile metrics (P50, P90, P95, P99, P99.9)
- Verify workload hash for replay validation

---

## 10. Error Handling

### 10.1 No Exceptions

Core MUST NOT use exceptions:

- All errors MUST be returned via result types
- `noexcept` is required on all public APIs
- Compile with `-fno-exceptions`

### 10.2 Result Types

```cpp
enum class frame_result : uint8_t {
    success = 0,
    invalid_phase_transition = 1,
    exceeded_time_budget = 2,
    allocation_failure = 3
};
```

All fallible operations MUST return explicit result codes.

---

## 11. Compilation Requirements

### 11.1 Language Standard

- C++23 (ISO/IEC 14882:2023) or later
- Minimum compiler versions with sufficient C++23 support:
  - GCC 13.0 or higher
  - Clang 18.0 or higher
  - MSVC 19.35 (Visual Studio 2022 17.5) or higher
- Standard library subset (no exceptions, no RTTI)
- Note: The specification uses C++23 features including:
  - `import std` modules (optional, traditional headers acceptable)
  - Range-based `std::expected` for error handling (optional)
  - Portable fixed-width integer types from `<cstdint>`

### 11.2 Compiler Flags

**Required:**
- `-std=c++23`
- `-fno-exceptions`
- `-fno-rtti`
- `-fno-fast-math` (preserves IEEE 754 determinism)
- `-ffp-contract=off` (disables FP contraction, ensures deterministic FP operations)
- `-Wall -Wextra -Werror`

**Recommended:**
- `-O3` (release builds)
- `-march=native` (architecture-specific optimizations, determinism preserved within same architecture)

### 11.3 Dependencies

**Allowed:**
- C++ standard library (subset: no iostreams, no exceptions)
- C standard library (subset: no I/O, no threads)

**Forbidden:**
- OS-specific headers
- Third-party libraries
- Dynamic linking

---

## 12. Acceptance Criteria

An implementation is conforming if:

1. ✅ **No core behavior depends on "implementation detail"**
   - All behavior is specified in this document
   - No undefined behavior in normal operation
   - No platform-specific assumptions

2. ✅ **A second engineer could reimplement core/ from spec alone**
   - This document is complete and unambiguous
   - All interfaces are precisely defined
   - All contracts are explicit

3. ✅ **All benchmarks still pass with a clean-room build**
   - Benchmarks verify determinism
   - Performance characteristics are preserved
   - Workload replay produces identical results

---

## 13. Version History

- **v1.0.0** (2025-12-15): Initial frozen specification
  - Frame lifecycle contract frozen
  - Memory management contract frozen
  - Determinism contract formalized (see DETERMINISM.md)

---

## 14. References

- [DETERMINISM.md](DETERMINISM.md) - Determinism guarantees and verification
- [ARCHITECTURE.md](ARCHITECTURE.md) - High-level system architecture
- [CORE_FOLDER_CONTRACT.md](CORE_FOLDER_CONTRACT.md) - Dependency boundaries
- [MEMORY_SYSTEM.md](MEMORY_SYSTEM.md) - Memory system implementation details
