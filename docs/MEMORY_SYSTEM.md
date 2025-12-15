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

## References

- [CORE_FOLDER_CONTRACT.md](../docs/CORE_FOLDER_CONTRACT.md) - Core memory section
- [frame_lifecycle.hpp](frame/frame_lifecycle.hpp) - Frame lifecycle integration
- [demo_allocators.cpp](memory/demo_allocators.cpp) - Example usage
