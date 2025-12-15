# Aegis Determinism Contract

**Status:** Frozen  
**Version:** 1.0.0  
**Last Updated:** 2025-12-15

---

## 1. Overview

This document defines the determinism guarantees of the Aegis Core Runtime. Determinism is a first-class architectural principle: **same inputs must produce byte-identical outputs**.

### 1.1 Why Determinism Matters

Determinism enables:

1. **Reproducible performance measurement**: Benchmarks can be replayed exactly
2. **Debugging and testing**: Bugs can be reproduced reliably
3. **Audit and compliance**: Behavior can be verified and proven
4. **Predictable execution**: No surprises, no hidden state, no timing dependencies
5. **Cross-platform verification**: Outputs are identical across platforms

### 1.2 Scope of Determinism

This contract applies to:

- All code in `core/` (mandatory)
- Benchmarking and verification tools (mandatory)
- Application code using core runtime (strongly recommended)

This contract does NOT apply to:

- `runtime/` platform layer (OS interactions are inherently non-deterministic)
- Network I/O
- User input (though input events are processed deterministically once received)

---

## 2. Fundamental Determinism Guarantee

### 2.1 The Core Invariant

**For any conforming implementation:**

```
If:
  - Initial state S₀ is identical (byte-for-byte)
  - Event sequence E = [e₁, e₂, ..., eₙ] is identical (byte-for-byte)
  - Timestamps T = [t₁, t₂, ..., tₙ] are identical

Then:
  - Final state Sₙ MUST be identical (byte-for-byte)
  - All intermediate states MUST be identical (byte-for-byte)
  - All generated outputs MUST be identical (byte-for-byte)
  - All memory allocations MUST occur in identical patterns
  - All timing measurements MUST be identical
```

### 2.2 Byte-Identical Definition

"Byte-identical" means:

- Memory layouts are identical
- Padding bytes are zero-initialized
- Pointer values are deterministic (relative, not absolute)
- Floating-point results are bit-exact
- No undefined behavior in comparisons

---

## 3. Sources of Determinism

### 3.1 Explicit Inputs Only

All inputs MUST be explicitly provided:

```cpp
// ✅ CORRECT: Explicit timestamp
frame_context.begin_frame(1234567890);

// ❌ WRONG: Implicit system clock
frame_context.begin_frame(system_clock::now()); // FORBIDDEN in core/
```

**MUST:**
- Accept all timestamps from caller
- Accept all events from caller
- Accept all buffers from caller

**MUST NOT:**
- Query system time
- Read environment variables
- Access global state
- Use thread-local storage

### 3.2 Deterministic Data Structures

All core data structures MUST be deterministic:

**POD Types:**
```cpp
struct deterministic_data {
    uint64_t id;
    float x, y;
    uint32_t color;
    // Padding MUST be zero-initialized
    uint32_t _padding{0};
};
```

**Containers:**
- Use array-based containers (deterministic iteration order)
- Avoid hash maps (unless hash function is deterministic and seeded explicitly)
- Sort before iteration if order matters

**MUST:**
- Zero-initialize all padding bytes
- Use stable sorting algorithms
- Maintain deterministic iteration order

**MUST NOT:**
- Use address-based hashing
- Rely on pointer comparison for ordering
- Use implementation-defined behavior

### 3.3 Deterministic Algorithms

All algorithms MUST be deterministic:

**Sorting:**
```cpp
// ✅ CORRECT: Stable sort with deterministic comparator
std::stable_sort(begin, end, [](const auto& a, const auto& b) {
    if (a.timestamp != b.timestamp) return a.timestamp < b.timestamp;
    return a.id < b.id; // Tie-breaker MUST be deterministic
});

// ❌ WRONG: Unstable sort without tie-breaker
std::sort(begin, end, [](const auto& a, const auto& b) {
    return a.timestamp < b.timestamp; // Undefined for equal timestamps
});
```

**MUST:**
- Use stable algorithms or deterministic tie-breakers
- Document algorithm complexity and behavior
- Avoid heuristic or approximate algorithms

---

## 4. Floating-Point Determinism

### 4.1 IEEE 754 Compliance

Floating-point operations MUST be deterministic:

**MUST:**
- Use IEEE 754 semantics
- Disable fast-math optimizations that break determinism
- Use consistent rounding modes
- Handle NaN and infinity consistently

**Compiler Flags:**
```bash
# ✅ CORRECT: Preserves determinism (GCC/Clang)
-fno-fast-math -ffp-contract=off

# ✅ CORRECT: Preserves determinism (MSVC)
/fp:strict

# ❌ WRONG: Breaks determinism
-ffast-math -funsafe-math-optimizations  # GCC/Clang
/fp:fast                                 # MSVC
```

### 4.2 Floating-Point Operations

**Safe Operations:**
```cpp
// ✅ CORRECT: Deterministic operations
float sum = a + b;
float product = a * b;
float result = std::sqrt(x); // IEEE 754 required
```

**Unsafe Operations:**
```cpp
// ⚠️ CAREFUL: Platform-specific behavior
float result = std::sin(x);  // May vary across platforms
float result = std::exp(x);  // May vary across platforms
```

**Mitigation:**
- Use fixed-point arithmetic where possible
- Document floating-point operations that may vary
- Provide deterministic reference implementations for transcendental functions if needed

### 4.3 Numerical Stability

**MUST:**
- Avoid catastrophic cancellation
- Use numerically stable algorithms
- Document precision requirements

**Example:**
```cpp
// ✅ CORRECT: Numerically stable
float mean = sum / count;
float variance = sum_squares / count - mean * mean;

// ❌ WRONG: Can lose precision
float variance = (sum_squares - sum * sum / count) / count;
```

---

## 5. Memory Determinism

### 5.1 Allocation Patterns

Memory allocation MUST be deterministic:

**MUST:**
- Allocate in identical patterns for identical inputs
- Use deterministic allocators (arena, frame, pool)
- Track all allocations explicitly

**Example:**
```cpp
// Frame N with events [e1, e2, e3]
allocator.allocate(64);  // For event e1
allocator.allocate(128); // For event e2
allocator.allocate(64);  // For event e3

// Replay of Frame N with events [e1, e2, e3]
allocator.allocate(64);  // MUST allocate same size
allocator.allocate(128); // MUST allocate same size
allocator.allocate(64);  // MUST allocate same size
```

### 5.2 Pointer Determinism

Absolute pointer values are non-deterministic, but relative offsets are:

**MUST:**
- Use indices or IDs instead of raw pointers for serialization
- Compare data values, not pointer values
- Use offset-based addressing where pointers are needed

**Example:**
```cpp
// ✅ CORRECT: ID-based reference
struct node {
    uint64_t id;
    uint64_t parent_id; // References parent by ID
};

// ❌ WRONG: Pointer-based reference
struct node {
    uint64_t id;
    node* parent; // Non-deterministic for comparison
};
```

### 5.3 Initialization

All memory MUST be initialized deterministically:

**MUST:**
- Zero-initialize all allocations
- Initialize all struct members explicitly
- Zero padding bytes

**Example:**
```cpp
// ✅ CORRECT: Explicit initialization
struct data {
    uint32_t value{0};
    uint32_t _padding{0};
};

// ❌ WRONG: Uninitialized
struct data {
    uint32_t value; // Uninitialized!
};
```

---

## 6. Event Determinism

### 6.1 Event Ordering

Events MUST be processed in deterministic order:

**Primary Sort Key:** Timestamp (ascending)
**Secondary Sort Key:** Sequence number (ascending)

```cpp
struct event {
    uint64_t timestamp_ns;
    uint64_t sequence_number;
    // ... event data
};

// Events MUST be sorted by timestamp, then sequence
void sort_events(std::vector<event>& events) {
    std::stable_sort(events.begin(), events.end(), [](const auto& a, const auto& b) {
        if (a.timestamp_ns != b.timestamp_ns)
            return a.timestamp_ns < b.timestamp_ns;
        return a.sequence_number < b.sequence_number;
    });
}
```

### 6.2 Event Timestamps

**MUST:**
- Express timestamps relative to frame start
- Use nanosecond precision (uint64_t)
- Provide monotonically increasing timestamps

**Example:**
```cpp
// Frame starts at T=1000000000
frame_context.begin_frame(1000000000);

// Events during frame
event e1{.timestamp_ns = 1000001000, .sequence_number = 0}; // +1000 ns
event e2{.timestamp_ns = 1000002000, .sequence_number = 1}; // +2000 ns
```

### 6.3 Event Replay

Workload replay MUST produce identical results:

```cpp
// Record workload
workload_recorder recorder;
recorder.start_recording("test_workload");
for (const auto& event : events) {
    recorder.record_event(event.type, event.timestamp, event.data);
}
workload work = recorder.finish_recording();
uint64_t original_hash = work.hash;

// Replay workload
workload_player player(work);
while (player.has_more_events()) {
    const workload_event* evt = player.next_event();
    // Process event
}
uint64_t replay_hash = player.get_hash();

// MUST be identical
assert(original_hash == replay_hash);
```

---

## 7. Timing Determinism

### 7.1 Timestamp Sources

**MUST:**
- Accept all timestamps from caller (via parameters)
- Never query system time in core/

**Example:**
```cpp
// ✅ CORRECT: Explicit timestamp from caller
void update(uint64_t timestamp_ns) {
    // Use timestamp_ns
}

// ❌ WRONG: Implicit system time
void update() {
    auto now = std::chrono::high_resolution_clock::now(); // FORBIDDEN
}
```

### 7.2 Time Budget Checking

Time budget enforcement MUST be deterministic:

```cpp
// Frame timing
uint64_t frame_start = 1000000000;
uint64_t frame_end   = 1000015000; // 15,000 ns elapsed
uint64_t frame_time  = frame_end - frame_start; // 15,000 ns

// Budget check (deterministic)
uint64_t budget = 16666667; // 16.67 ms
bool within_budget = (frame_time <= budget); // true
```

### 7.3 Phase Timing

Each phase records start and end times:

```cpp
// Phase timing MUST be exact
phase_start = current_timestamp;
// ... execute phase ...
phase_end = current_timestamp;
phase_duration = phase_end - phase_start; // Exact delta
```

---

## 8. Randomness and Non-Determinism

### 8.1 Forbidden Sources of Randomness

**MUST NOT use:**
- `std::rand()` or `rand()`
- `std::random_device`
- System entropy sources
- Uninitialized memory
- Address space layout randomization (ASLR) values
- Thread IDs or process IDs

### 8.2 Deterministic Random Number Generation

If randomness is needed (e.g., for testing), use seeded PRNGs:

```cpp
// ✅ CORRECT: Seeded PRNG
class deterministic_rng {
    uint64_t state;
public:
    explicit deterministic_rng(uint64_t seed) : state(seed) {}
    
    uint64_t next() {
        // Deterministic PRNG (e.g., xorshift64)
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return state;
    }
};

// Usage
deterministic_rng rng(12345); // Fixed seed
uint64_t value = rng.next();  // Deterministic sequence
```

---

## 9. Testing Determinism

### 9.1 Replay Testing

Every frame MUST be replayable:

```cpp
// Test: Record and replay produce identical results
void test_deterministic_replay() {
    // Execute frame once
    frame_context ctx1;
    ctx1.begin_frame(T0);
    // ... execute all phases ...
    ctx1.end_frame();
    frame_stats stats1 = ctx1.stats_get();
    
    // Execute frame again with identical inputs
    frame_context ctx2;
    ctx2.begin_frame(T0);
    // ... execute all phases with IDENTICAL inputs ...
    ctx2.end_frame();
    frame_stats stats2 = ctx2.stats_get();
    
    // MUST be byte-identical
    assert(memcmp(&stats1, &stats2, sizeof(frame_stats)) == 0);
}
```

### 9.2 Workload Hashing

Workloads use FNV-1a hashing for verification:

```cpp
// FNV-1a hash (deterministic)
constexpr uint64_t FNV_OFFSET = 14695981039346656037ULL;
constexpr uint64_t FNV_PRIME = 1099511628211ULL;

uint64_t hash_workload(const uint8_t* data, size_t length) {
    uint64_t hash = FNV_OFFSET;
    for (size_t i = 0; i < length; ++i) {
        hash ^= data[i];
        hash *= FNV_PRIME;
    }
    return hash;
}
```

### 9.3 Cross-Platform Verification

Determinism MUST hold across platforms:

**Test Matrix:**
- Linux x86_64 (GCC, Clang)
- Linux ARM64 (GCC, Clang)
- macOS x86_64 (Clang)
- macOS ARM64 (Clang)

**Verification:**
```bash
# Run on platform A
./benchmark > results_a.txt

# Run on platform B
./benchmark > results_b.txt

# Results MUST be identical
diff results_a.txt results_b.txt
# (no output = identical)
```

---

## 10. Debugging Non-Determinism

### 10.1 Common Sources of Non-Determinism

| Source | Detection | Fix |
|--------|-----------|-----|
| System time | Grep for `clock`, `time`, `now` | Pass timestamps as parameters |
| Uninitialized memory | Valgrind, MemorySanitizer | Zero-initialize all allocations |
| Hash map iteration | Test with multiple runs | Use ordered containers or sort |
| Floating-point | Platform comparison | Use `-fno-fast-math`, document variance |
| Pointer comparison | Address sanitizer | Use IDs instead of pointers |
| Thread scheduling | ThreadSanitizer | Single-threaded or explicit ordering |

### 10.2 Determinism Audit Checklist

Before merging code, verify:

- [ ] No system time queries
- [ ] No uninitialized memory reads
- [ ] No address-based comparisons
- [ ] No hash maps without deterministic iteration
- [ ] No unstable sorting without tie-breakers
- [ ] No floating-point fast-math
- [ ] No thread-local storage
- [ ] No random number generation (or seeded PRNG only)
- [ ] All padding bytes zero-initialized
- [ ] All external inputs explicit

### 10.3 Automated Verification

```bash
# Static analysis for determinism violations
clang-tidy --checks='-*,concurrency-*,bugprone-*' core/**/*.cpp

# Runtime verification with reproducibility test
for i in {1..10}; do
    ./benchmark --seed 42 > run_$i.txt
done
# All run_*.txt MUST be identical

# Memory initialization check
valgrind --track-origins=yes ./benchmark
```

---

## 11. Performance and Determinism Trade-offs

### 11.1 When Determinism Costs Performance

Some optimizations break determinism:

| Optimization | Impact | Mitigation |
|--------------|--------|------------|
| Fast-math | Breaks IEEE 754 | Use `-fno-fast-math` |
| Unordered containers | Breaks iteration order | Use ordered containers or sort |
| Parallel execution | Breaks ordering | Single-threaded or deterministic scheduling |
| Approximation algorithms | Breaks exact results | Use exact algorithms |

**Philosophy:** Aegis prioritizes determinism over maximum performance.

### 11.2 Acceptable Performance Trade-offs

We accept these costs for determinism:

- **10-20% slower** than fast-math (acceptable for correctness)
- **Ordered containers** instead of hash maps (acceptable for predictability)
- **Single-threaded** execution in core (acceptable for simplicity)
- **Exact algorithms** instead of approximations (acceptable for verification)

---

## 12. Guarantees and Limitations

### 12.1 What We Guarantee

**100% Deterministic:**
- Frame execution with identical inputs
- State transitions
- Memory allocation patterns
- Event processing order
- Scene graph construction
- Diff generation

**Deterministic within IEEE 754:**
- Floating-point arithmetic
- Layout computation
- Numerical calculations

### 12.2 What We Do NOT Guarantee

**Non-Deterministic (Platform Layer):**
- System time queries
- User input events (before they enter core)
- GPU execution timing
- OS thread scheduling
- Network I/O

**Acceptable Variation:**
- Transcendental functions (sin, cos, exp) may vary slightly across platforms
- Last-bit differences in floating-point due to compiler/architecture
- Performance timing (measured time may vary, but logical results are identical)

---

## 13. Compliance and Verification

### 13.1 Conformance Tests

A conforming implementation MUST pass:

1. **Replay Test**: 1000 identical frames produce identical `frame_stats`
2. **Workload Hash Test**: Recorded and replayed workloads have matching hashes
3. **Cross-Platform Test**: Same workload produces same results on all platforms
4. **Memory Test**: Allocation patterns are identical across runs
5. **Event Test**: Event ordering is stable and deterministic

### 13.2 Continuous Verification

Every CI run MUST:

- Execute determinism tests
- Compare results across platforms
- Verify workload replay
- Check for non-deterministic sources

---

## 14. Migration and Evolution

### 14.1 Backward Compatibility

When the core runtime evolves:

**MUST:**
- Maintain determinism guarantees
- Provide migration path for workload replay
- Version workload formats
- Document any behavioral changes

**MUST NOT:**
- Break existing replay logs
- Change semantics without version bump
- Introduce non-determinism

### 14.2 Deprecation Policy

If a feature must be deprecated:

1. Announce deprecation with version
2. Provide deterministic alternative
3. Maintain compatibility for 2+ major versions
4. Document migration path

---

## 15. References and Further Reading

### 15.1 Related Documents

- [SPEC_CORE_V1.md](SPEC_CORE_V1.md) - Core runtime specification
- [ARCHITECTURE.md](ARCHITECTURE.md) - High-level architecture
- [MEMORY_SYSTEM.md](MEMORY_SYSTEM.md) - Memory management details

### 15.2 External References

- IEEE 754-2008 Floating-Point Standard
- ISO/IEC 14882:2023 C++ Standard
  - Section 6.8.1: Sequential execution (deterministic program execution)
  - Section 7.7: Fundamental types (fixed sizes via `<cstdint>`)
  - Annex C: Compatibility (stable ABI for POD types)
- "The Art of Computer Systems Performance Analysis" - Jain (1991)
- "Principles of Transaction Processing" - Bernstein & Newcomer (2009)

---

## 16. Version History

- **v1.0.0** (2025-12-15): Initial frozen specification
  - Fundamental determinism guarantee defined
  - Event ordering specified
  - Memory determinism formalized
  - Testing methodology established

---

## 17. Acceptance Criteria Summary

A deterministic implementation MUST:

1. ✅ Produce byte-identical outputs for identical inputs
2. ✅ Support workload recording and replay with hash verification
3. ✅ Pass cross-platform verification tests
4. ✅ Maintain deterministic memory allocation patterns
5. ✅ Process events in deterministic order
6. ✅ Avoid all sources of non-determinism listed in §8.1
7. ✅ Use deterministic floating-point (no fast-math)
8. ✅ Zero-initialize all memory and padding bytes
9. ✅ Accept all timing inputs explicitly (no system clocks)
10. ✅ Pass all conformance tests in §13.1
