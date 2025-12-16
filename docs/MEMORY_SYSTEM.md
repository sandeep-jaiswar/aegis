# Explicit Memory System Implementation

This document describes the explicit memory system implemented for Aegis, including arena, frame, and pool allocators.

## Overview

The memory system provides deterministic, measurable memory management with zero global new/delete allocations during frame execution. It consists of three specialized allocators:

1. **Arena Allocator**: Bulk allocation with efficient reset
2. **Frame Allocator**: Per-frame allocation with automatic reset
3. **Pool Allocator**: Fixed-size object allocation

## Design Principles

- **No global allocations**: All allocators use pre-allocated buffers
- **Measurable costs**: All allocations/deallocations are tracked
- **Deterministic**: Same inputs produce identical memory patterns
- **O(1) operations**: All critical operations are constant time
- **No RTTI/exceptions**: Complies with core folder contract

## Arena Allocator

### Purpose
Linear allocator for allocating many objects that are freed together.

### Characteristics
- **Allocation**: O(1) - bumps offset pointer
- **Deallocation**: No-op (tracked for statistics)
- **Reset**: O(1) - resets offset to zero
- **Use case**: Temporary allocations within a phase or subsystem

### Example Usage
```cpp
alignas(16) uint8_t buffer[4096];
arena_allocator arena(buffer, sizeof(buffer));

void* ptr1 = arena.allocate(128, 16);  // Allocate 128 bytes
void* ptr2 = arena.allocate(256, 16);  // Allocate 256 bytes

// ... use allocations ...

arena.reset();  // O(1) - frees all allocations
```

## Frame Allocator

### Purpose
Per-frame linear allocator optimized for allocations that live exactly one frame.

### Characteristics
- **Allocation**: O(1) - bumps offset pointer
- **Deallocation**: No-op (tracked for statistics)
- **Reset**: O(1) - automatically called at end_frame()
- **Integration**: Built into frame_context lifecycle
- **Use case**: Frame-local temporary data (UI state, layout results, etc.)

### Example Usage
```cpp
alignas(16) uint8_t buffer[2048];
frame_allocator frame_alloc(buffer, sizeof(buffer));
frame_context ctx(&frame_alloc);

// During frame execution
void* ptr = frame_alloc.allocate(64, 8);

// ... use allocation ...

ctx.end_frame();  // Automatically resets frame_alloc
```

### Lifecycle Integration

The frame allocator is integrated into the frame lifecycle:

```
begin_frame()
  ↓
[frame allocations happen here]
  ↓
end_frame()
  ↓
  1. Capture allocation statistics
  2. Update frame_stats
  3. Reset frame allocator (O(1))
  ↓
Frame allocator ready for next frame
```

## Pool Allocator

### Purpose
Fixed-size object allocation with free-list management.

### Characteristics
- **Allocation**: O(1) - pop from free list
- **Deallocation**: O(1) - push to free list
- **Reset**: O(n) where n = block_count (rebuilds free list)
- **Use case**: Many same-sized objects (nodes, events, etc.)

### Example Usage
```cpp
alignas(16) uint8_t buffer[1024];
pool_allocator pool(buffer, sizeof(buffer), 64, 8);  // 64-byte blocks

void* ptr1 = pool.allocate(1, 1);  // Size ignored
void* ptr2 = pool.allocate(1, 1);

pool.deallocate(ptr1, 64);  // Return to pool

void* ptr3 = pool.allocate(1, 1);  // Reuses ptr1
```

## Statistics and Monitoring

All allocators track:
- `bytes_allocated`: Total bytes allocated
- `bytes_freed`: Total bytes deallocated
- `bytes_in_use`: Currently allocated bytes
- `peak_bytes_used`: Maximum bytes used

Frame allocator additionally tracks:
- `allocation_count`: Number of allocations
- `deallocation_count`: Number of deallocations

These statistics are integrated into `frame_stats` for performance analysis.

## Acceptance Criteria

### 1. Zero calls to global new/delete during frame execution ✓

All allocators use pre-allocated buffers:
```cpp
// Pre-allocated buffers (stack or static)
alignas(16) uint8_t arena_buffer[4096];
alignas(16) uint8_t frame_buffer[2048];
alignas(16) uint8_t pool_buffer[1024];

// No heap allocations during frame execution
arena_allocator arena(arena_buffer, sizeof(arena_buffer));
frame_allocator frame_alloc(frame_buffer, sizeof(frame_buffer));
pool_allocator pool(pool_buffer, sizeof(pool_buffer), 64, 8);
```

### 2. Allocation/free cost measurable and logged ✓

All allocators provide statistics:
```cpp
printf("Bytes allocated: %zu\n", alloc.bytes_allocated());
printf("Bytes freed: %zu\n", alloc.bytes_freed());
printf("Peak bytes used: %zu\n", alloc.peak_bytes_used());
```

Frame allocator statistics are captured in `frame_stats`:
```cpp
struct frame_stats {
    // ... existing fields ...
    size_t bytes_allocated{0};
    size_t bytes_freed{0};
    size_t peak_memory_used{0};
    size_t frame_allocations{0};
    size_t frame_deallocations{0};
};
```

### 3. Frame allocator fully resets at end_frame() ✓

Automatic reset in frame lifecycle:
```cpp
frame_result frame_context::end_frame() noexcept {
    // ... phase timing ...
    
    if (frame_alloc != nullptr) {
        // Capture statistics
        stats.frame_allocations = frame_alloc->get_allocation_count();
        stats.frame_deallocations = frame_alloc->get_deallocation_count();
        
        // Update memory stats
        stats.bytes_allocated += frame_alloc->bytes_allocated();
        stats.bytes_freed += frame_alloc->bytes_freed();
        
        // Reset - O(1) operation
        frame_alloc->reset();
    }
    
    // ... budget check ...
}
```

## Memory Safety

### Overflow Protection
The `align_up` function includes overflow checks:
```cpp
inline size_t align_up(size_t value, size_t alignment) noexcept {
    // Overflow check
    if (alignment == 0 || value > (static_cast<size_t>(-1) - alignment + 1)) {
        return value;
    }
    return (value + alignment - 1) & ~(alignment - 1);
}
```

### Alignment Requirements
- All allocators support custom alignment
- Alignment must be a power of 2
- Default alignment is `alignof(max_align_t)`

### Buffer Lifetime
- Allocators do not own their buffers
- Buffers must remain valid for allocator lifetime
- Caller responsible for buffer allocation/deallocation

## Performance Characteristics

| Allocator | Allocate | Deallocate | Reset    | Memory Overhead |
|-----------|----------|------------|----------|-----------------|
| Arena     | O(1)     | O(1)*      | O(1)     | 0 bytes         |
| Frame     | O(1)     | O(1)*      | O(1)     | 0 bytes         |
| Pool      | O(1)     | O(1)       | O(n)**   | 0 bytes***      |

\* Deallocation is a no-op but tracked for statistics
\** Where n = block_count
\*** After initialization; initialization requires space for free list pointers

## Testing

A demonstration program (`memory_demo`) validates all acceptance criteria:

```bash
# Build and run
cmake --build build
./build/core/memory_demo
```

Output shows:
- Zero global allocations
- Complete allocation statistics
- Frame allocator reset behavior
- Memory reuse after reset

## Future Enhancements

Potential improvements:
1. **Thread-safe variants**: Lock-free allocators for concurrent access
2. **Memory budgets**: Hard limits with failure callbacks
3. **Allocation scopes**: RAII-style scope guards
4. **Memory tagging**: Debug builds with allocation tracking
5. **Alignment validation**: Debug assertions for power-of-2 check

---

## Structural Sharing Guarantees

Aegis supports structural sharing for immutable state management with strict lifetime guarantees.

### Principles

1. **Immutability**: Shared structures are never mutated
2. **Reference Counting**: Optional for complex data structures
3. **Copy-on-Write**: When mutation is needed, create new version
4. **Deterministic Sharing**: Same inputs lead to same sharing patterns

### Safe Sharing Patterns

**Immutable Scene Nodes:**
```cpp
// Previous frame's scene graph (immutable)
const scene_graph prev_frame;

// Current frame builds new scene
scene_graph curr_frame;

// Safe to reference prev_frame data (read-only)
for (const auto& node : prev_frame.nodes) {
    // Copy node data (safe because immutable)
    curr_frame.add_node(node);
}

// After diff, prev_frame can be freed
```

**State Snapshots:**
```cpp
// State snapshots are immutable
struct state_snapshot {
    // All data is value-type or managed separately
    uint64_t timestamp;
    std::vector<entity> entities;  // Copied, not shared
    
    // Safe copy constructor (deep copy)
    state_snapshot(const state_snapshot& other) = default;
};

// Previous state remains valid
state_snapshot prev_state = current_state;
// New state is independent
state_snapshot next_state = prev_state.apply_event(evt);
```

**Arena-Backed Sharing:**
```cpp
// Arena allocator enables efficient structural sharing
arena_allocator state_arena(buffer, size);

// Allocate state from arena
state_snapshot* s1 = allocate_state(state_arena);

// Can share pointers within same arena lifetime
state_snapshot* s2 = s1;  // Both valid until arena reset

// Reset invalidates all pointers
state_arena.reset();  // s1 and s2 now invalid
```

### Forbidden Sharing Patterns

**FORBIDDEN: Sharing across frame allocators**
```cpp
// ❌ WRONG: Pointer from frame N used in frame N+1
frame_allocator frame_alloc;
void* ptr = frame_alloc.allocate(64, 8);  // Frame N
// ... end_frame() resets allocator ...
// ptr is now INVALID (undefined behavior to use)
use_data(ptr);  // ❌ Dangling pointer
```

**FORBIDDEN: Mutable shared state**
```cpp
// ❌ WRONG: Multiple owners of mutable data
struct mutable_node {
    int value;
};

mutable_node* shared = new mutable_node{42};
node_a.data = shared;
node_b.data = shared;

node_a.data->value = 100;  // ❌ Affects node_b (non-deterministic)
```

**FORBIDDEN: Circular references**
```cpp
// ❌ WRONG: Circular ownership
struct node {
    node* parent;
    node* child;
};

node* a = allocate_node();
node* b = allocate_node();
a->child = b;
b->parent = a;  // ❌ Circular reference (leak without GC)
```

### Memory Lifetime Rules

**Rule 1: Frame Allocator Lifetime**
- Allocations MUST NOT outlive the frame
- Pointers become invalid at end_frame()
- No references to frame allocations in next frame

**Rule 2: Arena Allocator Lifetime**
- Allocations valid until arena.reset()
- Multiple objects can share arena
- Arena owner controls lifetime

**Rule 3: State Snapshot Lifetime**
- Snapshots are value types (deep copy)
- No shared mutable state between snapshots
- Previous snapshots remain valid

**Rule 4: Scene Graph Lifetime**
- Previous frame's scene is immutable
- Current frame's scene is under construction
- Diff operation compares two immutable scenes

### Verification

Lifetime safety is verified by:

1. **Static Analysis**: No pointers stored across frame boundaries
2. **Runtime Checks**: Arena/frame allocator asserts on invalid access
3. **Determinism Tests**: Same inputs produce same allocation patterns
4. **Memory Sanitizers**: AddressSanitizer detects use-after-free

---

## Forbidden Allocation Patterns

This section explicitly lists allocation patterns that MUST NOT be used in conforming implementations.

### Pattern 1: Global new/delete in Frame Execution

**FORBIDDEN:**
```cpp
void on_frame_update() {
    auto* data = new DataObject();  // ❌ Global allocation
    process(data);
    delete data;  // ❌ Global deallocation
}
```

**CORRECT:**
```cpp
void on_frame_update(frame_allocator& alloc) {
    auto* data = alloc.allocate(sizeof(DataObject), alignof(DataObject));
    process(data);
    // No explicit delete - allocator resets at frame end
}
```

### Pattern 2: Allocation Without Size Tracking

**FORBIDDEN:**
```cpp
class bad_allocator {
    void* allocate(size_t size) {
        return malloc(size);  // ❌ No tracking
    }
};
```

**CORRECT:**
```cpp
class good_allocator {
    void* allocate(size_t size, size_t alignment) {
        total_allocated += size;  // ✅ Track allocation
        return bump_allocate(size, alignment);
    }
};
```

### Pattern 3: Hidden Allocations in Containers

**FORBIDDEN:**
```cpp
void process_events(const std::vector<event>& events) {
    std::vector<event> filtered;  // ❌ Hidden heap allocation
    for (const auto& e : events) {
        if (e.type == desired_type)
            filtered.push_back(e);  // ❌ Potential reallocation
    }
}
```

**CORRECT:**
```cpp
void process_events(const std::vector<event>& events, frame_allocator& alloc) {
    // Pre-allocate buffer from frame allocator
    event* filtered = static_cast<event*>(
        alloc.allocate(events.size() * sizeof(event), alignof(event))
    );
    size_t count = 0;
    for (const auto& e : events) {
        if (e.type == desired_type)
            filtered[count++] = e;
    }
}
```

### Pattern 4: Indefinite Lifetime Allocations

**FORBIDDEN:**
```cpp
// Allocation that never gets freed
static std::vector<event> event_log;  // ❌ Grows unbounded

void log_event(const event& e) {
    event_log.push_back(e);  // ❌ Never freed
}
```

**CORRECT:**
```cpp
// Bounded allocation with explicit lifetime
class event_log {
    event* buffer;
    size_t capacity;
    size_t count;
    
public:
    event_log(arena_allocator& arena, size_t max_events)
        : buffer(static_cast<event*>(
              arena.allocate(max_events * sizeof(event), alignof(event)))),
          capacity(max_events),
          count(0) {}
    
    void add(const event& e) {
        if (count < capacity)
            buffer[count++] = e;
    }
    
    void clear() { count = 0; }  // Reuse buffer
};
```

### Pattern 5: Allocation in Destructors

**FORBIDDEN:**
```cpp
class bad_object {
    ~bad_object() {
        cleanup_data = new uint8_t[1024];  // ❌ Allocation in destructor
    }
};
```

**CORRECT:**
```cpp
class good_object {
    ~good_object() noexcept {
        // No allocations - only cleanup of existing resources
    }
};
```

### Pattern 6: Recursive Unbounded Allocations

**FORBIDDEN:**
```cpp
void recursive_process(const node* n) {
    auto* copy = new node(*n);  // ❌ Unbounded recursion + allocation
    if (n->left) recursive_process(n->left);
    if (n->right) recursive_process(n->right);
}
```

**CORRECT:**
```cpp
void iterative_process(const node* root, arena_allocator& arena) {
    // Pre-allocate stack space
    const node** stack = static_cast<const node**>(
        arena.allocate(MAX_DEPTH * sizeof(node*), alignof(node*))
    );
    size_t stack_size = 0;
    
    stack[stack_size++] = root;
    while (stack_size > 0) {
        const node* n = stack[--stack_size];
        // Process bounded, no recursion
    }
}
```

### Pattern 7: Memory Leaks via Lost Pointers

**FORBIDDEN:**
```cpp
void process_frame(arena_allocator& arena) {
    void* data = arena.allocate(1024, 16);
    // ... forgot to track pointer ...
    data = arena.allocate(2048, 16);  // ❌ Lost first allocation
}
```

**CORRECT:**
```cpp
void process_frame(arena_allocator& arena) {
    struct frame_data {
        void* buffer1;
        void* buffer2;
    };
    
    frame_data data;
    data.buffer1 = arena.allocate(1024, 16);
    data.buffer2 = arena.allocate(2048, 16);
    // Both allocations tracked
}
```

---

## Memory Behavior Reproducibility Under Replay

All memory operations MUST be reproducible during replay for determinism.

### Replay Guarantees

**Given:**
- Identical initial state S₀
- Identical event sequence E = [e₁, e₂, ..., eₙ]
- Identical allocator configurations

**Then:**
- Allocation order MUST be identical
- Allocation sizes MUST be identical
- Allocation alignments MUST be identical
- Memory layout MUST be identical
- Peak memory usage MUST be identical

### Replay Verification Test

```cpp
// First execution - record
frame_allocator alloc1(buffer, sizeof(buffer));
frame_context ctx1(&alloc1);

ctx1.begin_frame(T0);
ctx1.apply_events();  // Process event e1
void* ptr1_run1 = alloc1.allocate(64, 8);
ctx1.end_frame();

frame_stats stats1 = ctx1.stats_get();

// Second execution - replay
frame_allocator alloc2(buffer, sizeof(buffer));
frame_context ctx2(&alloc2);

ctx2.begin_frame(T0);
ctx2.apply_events();  // Process same event e1
void* ptr1_run2 = alloc2.allocate(64, 8);
ctx2.end_frame();

frame_stats stats2 = ctx2.stats_get();

// Verify reproducibility
assert(stats1.bytes_allocated == stats2.bytes_allocated);
assert(stats1.bytes_freed == stats2.bytes_freed);
assert(stats1.peak_memory_used == stats2.peak_memory_used);
assert(stats1.frame_allocations == stats2.frame_allocations);
```

### Deterministic Allocation Tracking

All allocations MUST be tracked deterministically:

```cpp
class allocation_tracker {
    struct allocation_record {
        uint64_t timestamp_ns;
        size_t size;
        size_t alignment;
        const char* source_file;
        int source_line;
    };
    
    std::vector<allocation_record> allocations;
    
public:
    void record_allocation(uint64_t timestamp, size_t size, 
                          size_t alignment, 
                          const char* file, int line) {
        allocations.push_back({timestamp, size, alignment, file, line});
    }
    
    // Compute deterministic hash
    uint64_t compute_hash() const {
        uint64_t hash = FNV_OFFSET;
        for (const auto& record : allocations) {
            hash ^= record.timestamp_ns;
            hash *= FNV_PRIME;
            hash ^= record.size;
            hash *= FNV_PRIME;
            hash ^= record.alignment;
            hash *= FNV_PRIME;
        }
        return hash;
    }
};
```

### Cross-Platform Memory Reproducibility

Memory behavior MUST be identical across platforms:

| Platform | Pointer Size | Alignment | Determinism |
|----------|--------------|-----------|-------------|
| Linux x64 | 8 bytes | alignof(max_align_t) | ✅ Yes |
| Linux ARM64 | 8 bytes | alignof(max_align_t) | ✅ Yes |
| macOS x64 | 8 bytes | alignof(max_align_t) | ✅ Yes |
| macOS ARM64 | 8 bytes | alignof(max_align_t) | ✅ Yes |

**Verification:**
```bash
# Run on platform A
./benchmark --record memory_test.log

# Run on platform B
./benchmark --replay memory_test.log

# Results MUST match
diff platform_a_stats.txt platform_b_stats.txt
# (no output = identical)
```

---

## References

- [CORE_FOLDER_CONTRACT.md](../docs/CORE_FOLDER_CONTRACT.md) - Core memory section
- [frame_lifecycle.hpp](frame/frame_lifecycle.hpp) - Frame lifecycle integration
- [demo_allocators.cpp](memory/demo_allocators.cpp) - Example usage
