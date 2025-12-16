# External Falsification Guide

## Purpose

External falsification is about **deliberately challenging** Aegis's architectural claims by implementing the same workload using alternative technologies. This is not marketing - it's honest engineering validation.

## The Philosophy

From the issue:

> "Do this deliberately... If Aegis doesn't win here, you revise — not rationalize."

We measure:
- **P99 frame time**: Tail latency matters more than average
- **Frame variance**: Predictability is the core claim
- **Memory stability**: Allocation patterns should be deterministic
- **Replay fidelity**: Can we reproduce results exactly?

## The Implementations

### 1. Aegis (Baseline)

**Technology**: C++ with custom frame allocator
**Path**: `../core/benchmark/browser_comparison_benchmark.hpp`

**Characteristics**:
- Explicit memory management (frame allocator)
- Deterministic execution
- No garbage collection
- No JIT compilation
- Direct execution, no framework overhead

**Expected strengths**:
- ✅ Extremely low variance
- ✅ Predictable P99 latency
- ✅ Perfect replay fidelity
- ✅ Deterministic memory patterns

**Expected weaknesses**:
- May not be fastest on average (no JIT optimizations)
- Requires more manual memory management
- Less developer ergonomics than high-level frameworks

### 2. React (Chromium + React)

**Technology**: React 18 with virtual DOM
**Path**: `react/`

**Characteristics**:
- Virtual DOM reconciliation
- Component lifecycle overhead
- React Fiber scheduler
- JavaScript garbage collection
- Browser event loop

**Expected strengths**:
- ✅ Developer productivity
- ✅ Rich ecosystem
- ⚠️ May have good average performance (JIT)

**Expected weaknesses**:
- ❌ High variance (GC pauses, React scheduling)
- ❌ Poor P99 (tail latency from framework)
- ❌ No replay fidelity (non-deterministic GC)

### 3. Canvas (Chromium + Canvas)

**Technology**: HTML5 Canvas 2D API
**Path**: `canvas/`

**Characteristics**:
- Direct 2D drawing API
- GPU rasterization
- Browser rendering pipeline
- JavaScript runtime overhead

**Expected strengths**:
- ✅ Good for graphics workloads
- ✅ Hardware acceleration
- ⚠️ Can be fast for rendering

**Expected weaknesses**:
- ❌ Variance from GPU scheduling
- ❌ Browser pipeline overhead
- ❌ JavaScript GC impact
- ❌ Platform-dependent GPU behavior

### 4. Qt (Native)

**Technology**: Qt 6 with native widgets
**Path**: `qt/`

**Characteristics**:
- Native C++ code
- Qt event loop
- Platform-specific rendering
- Standard C++ allocators

**Expected strengths**:
- ✅ Native performance
- ✅ No GC pauses
- ✅ Good average performance
- ⚠️ Better than browser, worse than Aegis

**Expected weaknesses**:
- ❌ Qt event loop variance
- ❌ Platform windowing system overhead
- ❌ Some non-determinism
- ⚠️ Framework overhead (less than React)

## Running Complete Comparison

### Step 1: Build and Run Aegis

```bash
cd /path/to/aegis
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/core/browser_comparison_demo | python3 external_falsification/convert_aegis_output.py > external_falsification/aegis_results.json
```

### Step 2: Run React Benchmark

```bash
cd external_falsification/react
npm install
npm run dev
```

Then:
1. Open http://localhost:3000
2. Click "Start Benchmark"
3. Wait for completion
4. Click "Export Results"
5. Save as `react_results.json` in `external_falsification/`

### Step 3: Run Canvas Benchmark

```bash
cd external_falsification/canvas
# Open canvas_benchmark.html in Chrome
```

Then:
1. Click "Start Benchmark"
2. Wait for completion
3. Click "Export Results"
4. Save as `canvas_results.json` in `external_falsification/`

### Step 4: Run Qt Benchmark

```bash
cd external_falsification/qt
mkdir -p build && cd build
cmake ..
make
./qt_benchmark
```

Then:
1. Click "Start Benchmark" in GUI
2. Wait for completion
3. Click "Export Results"
4. Save as `qt_results.json` in `external_falsification/`

### Step 5: Analyze Results

```bash
cd external_falsification
python3 analyze_results.py
```

## Interpreting Results

### Variance Analysis

**Key Question**: Does Aegis have order-of-magnitude better variance?

**Success Criteria**:
- Aegis variance should be 10-100x lower than browser stacks
- Aegis variance should be 2-10x lower than Qt

**If Aegis loses**:
- Investigate why (GC? OS scheduling? Measurement error?)
- Fix the implementation, don't rationalize
- Document honest findings

### P99 Latency Analysis

**Key Question**: Is Aegis's tail latency predictably low?

**Success Criteria**:
- Aegis P99 should be close to Aegis P50 (tight distribution)
- Other stacks should have wider P99-P50 spreads

**Red flags**:
- If Aegis P99 is far from P50: variance problem
- If other stacks have tighter distribution: architecture problem

### Honest Assessment Checklist

When analyzing results, be honest:

✅ **Where Aegis wins**: Document clearly with metrics
✅ **Where Aegis loses**: Explain why (architecture, measurement, etc.)
✅ **Where results are mixed**: Provide nuanced analysis
❌ **Don't rationalize**: If Aegis loses, fix it or explain honestly

## Common Pitfalls

### 1. Measurement Bias

**Problem**: Running benchmarks in different conditions
**Solution**: 
- Close all other applications
- Run each benchmark 3 times, take median
- Use same hardware for all tests

### 2. Workload Mismatch

**Problem**: Implementations don't match exactly
**Solution**:
- Verify all implement identical workload (50 allocs, 100 ops)
- Check allocation sizes match
- Ensure same iteration counts

### 3. Cherry-picking Results

**Problem**: Only showing favorable metrics
**Solution**:
- Report all metrics (min, P50, P90, P95, P99, max, variance)
- Include results where Aegis loses
- Explain all findings honestly

## Expected Outcomes

Based on Aegis's architecture:

### Aegis Should Win On:

1. **Variance** (primary claim)
   - Frame allocator eliminates GC variance
   - Deterministic execution eliminates JIT variance
   - Expected: 10-100x better than browsers, 2-10x better than Qt

2. **P99 Predictability**
   - Tight tail latency distribution
   - Expected: P99/P50 ratio close to 1.0

3. **Replay Fidelity**
   - Byte-for-byte reproducible
   - Expected: Perfect reproduction with same inputs

### Aegis May Lose On:

1. **Average Performance**
   - JIT can optimize hot paths better
   - GPU acceleration can be fast
   - This is acceptable - we optimize for predictability, not peak speed

2. **Development Velocity**
   - React/Qt have better tooling
   - Higher-level abstractions
   - This is expected - we trade ease for control

### Red Flags:

- ❌ If browser stacks have better variance: Major architecture problem
- ❌ If Qt has better P99: Need to investigate Aegis overhead
- ❌ If replay isn't perfect: Determinism contract violation

## Conclusion

External falsification is about **validation through challenge**. We want to prove Aegis's claims hold up under scrutiny, or learn where they don't and improve.

The goal is not marketing material - it's honest engineering.
