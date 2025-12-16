# External Validation Guide

This document provides a step-by-step guide for external reviewers to build, test, and validate Aegis.

**Purpose:** Enable independent verification of all claims made in the documentation and specifications.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Prerequisites Verification](#prerequisites-verification)
3. [Building from Source](#building-from-source)
4. [Running Tests](#running-tests)
5. [Validating Specifications](#validating-specifications)
6. [Validating Performance Claims](#validating-performance-claims)
7. [Validating Determinism](#validating-determinism)
8. [Validating Documentation](#validating-documentation)
9. [Common Issues](#common-issues)

---

## Quick Start

**5-Minute Validation:**

```bash
# 1. Clone repository
git clone https://github.com/sandeep-jaiswar/aegis.git
cd aegis

# 2. Build (Release mode)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 3. Run conformance tests
./build/runtime/conformance_tests/conformance_tests

# 4. Run capability tests
./build/core/module/capability_test

# Expected result: All tests pass ✅
```

---

## Prerequisites Verification

### Verify CMake Version

```bash
cmake --version
# Required: 3.20 or higher
```

**If version is too old:**
- Ubuntu/Debian: `sudo apt update && sudo apt install cmake`
- Or download from: https://cmake.org/download/

### Verify Compiler Version

**Check GCC:**
```bash
g++ --version
# Required: GCC 13.0 or higher
```

**Check Clang:**
```bash
clang++ --version
# Required: Clang 18.0 or higher
```

**If compiler is too old:**
- Ubuntu 24.04+: Includes GCC 13+ by default
- Ubuntu 22.04: Install newer GCC from toolchain PPA
- Or use Clang 18+ from LLVM project

### Verify Git

```bash
git --version
# Any recent version is fine
```

---

## Building from Source

### 1. Clone Repository

```bash
git clone https://github.com/sandeep-jaiswar/aegis.git
cd aegis
```

**Verify clone:**
```bash
ls -la
# Should see: BUILD.md, CMakeLists.txt, core/, docs/, LICENSE, README.md
```

### 2. Configure Build (Release Mode)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

**Expected output:**
```
-- The CXX compiler identification is GNU 13.3.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- clang-tidy enabled for aegis_core
-- Configuring done (0.3s)
-- Generating done (0.0s)
-- Build files have been written to: /path/to/aegis/build
```

### 3. Build

```bash
cmake --build build
```

**Expected output:**
```
[ 1%] Building CXX object core/CMakeFiles/aegis_core.dir/...
...
[100%] Built target aegis
```

**Should see 0 warnings** (build uses `-Werror`)

### 4. Verify Build Artifacts

```bash
ls -lh build/core/libaegis_core.a
ls -lh build/runtime/conformance_tests/conformance_tests
ls -lh build/core/module/capability_test
```

**All files should exist.**

### Alternative: Debug Build

```bash
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

### Alternative: Using Clang

```bash
cmake -B build-clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build-clang
```

---

## Running Tests

### 1. Run Conformance Test Suite

```bash
./build/runtime/conformance_tests/conformance_tests
```

**Expected output:**
```
╔═════════════════════════════════════════════════════════════════╗
║  Aegis Runtime Conformance Test Suite                          ║
║  Ticket: REF-002                                                ║
╚═════════════════════════════════════════════════════════════════╝

=================================================================
    Aegis Runtime Conformance Test Suite (REF-002)
=================================================================

[1/29] Frame execution is determinism
Test: Frame execution determinism
Invariant: Same inputs → same frame stats
  ✓ PASSED

...

Test Results:
  Total:           29
  ✓ Passed:        16
  ✗ Failed:        0
  ⊘ Not Impl:      10
  ⊙ Skipped:       3

Status: ✅ ALL TESTS PASSED
=================================================================
```

**Validation:**
- ✅ 16 tests should pass
- ✅ 0 tests should fail
- ⊘ 10 tests marked "NOT IMPLEMENTED" (future enhancements)
- ⊙ 3 tests marked "SKIPPED" (multi-platform validation)

### 2. Run Capability Tests

```bash
./build/core/module/capability_test
```

**Expected output:**
```
=== Aegis Capability Model Tests ===

Test 1: Module with no capabilities...
  ✓ Module loads on runtime with no capabilities
  ✓ Module loads on runtime with all capabilities
  ✅ Test 1 passed

...

Test 6: Capability bit operations...
  ✓ has_capability() works correctly
  ✓ Bitwise OR combines capabilities
  ✓ Bitwise masking removes capabilities
  ✅ Test 6 passed

=== All Tests Passed ✅ ===

Acceptance Criteria Verified:
  ✅ No ambient authority exists
  ✅ Missing capability → deterministic failure
  ✅ Capability enforcement is testable
  ✅ Capability enforcement is replayable
```

**Validation:**
- ✅ All 6 tests should pass
- ✅ All acceptance criteria should be verified

### 3. Run Specific Test Categories

```bash
# Run only determinism tests
./build/runtime/conformance_tests/conformance_tests determinism

# Run only memory tests
./build/runtime/conformance_tests/conformance_tests memory

# Run only capability tests
./build/runtime/conformance_tests/conformance_tests capability
```

### 4. Run Demo Programs

```bash
# Memory allocator demo
./build/core/memory_demo

# Immutable state demo
./build/core/state_demo

# Event stream demo
./build/core/event_stream_demo

# Layout engine demo
./build/core/layout_demo

# Scene graph demo
./build/core/scene_graph_demo

# Benchmark harness demo
./build/core/benchmark_demo

# Browser comparison demo
./build/core/browser_comparison_demo
```

**Each demo should:**
- Run without errors
- Produce deterministic output
- Complete successfully

---

## Validating Specifications

### 1. Check Specification Status

```bash
cat docs/SPECIFICATION_INDEX.md | grep "Status:"
```

**Expected output:**
```
**Status:** Frozen
**Status:** Frozen
**Status:** Frozen
...
```

**Validation:**
- ✅ All specifications should be marked "Frozen" or "Published"
- ✅ Version should be 1.0.0

### 2. Verify Specification Completeness

**Check that each core module has a specification:**

```bash
# Core memory system
grep -A 5 "core/memory/" docs/SPECIFICATION_INDEX.md

# Core events system
grep -A 5 "core/events/" docs/SPECIFICATION_INDEX.md

# Core state system
grep -A 5 "core/state/" docs/SPECIFICATION_INDEX.md

# Core layout system
grep -A 5 "core/layout/" docs/SPECIFICATION_INDEX.md

# Core frame system
grep -A 5 "core/frame/" docs/SPECIFICATION_INDEX.md
```

**Each should show:**
- Specification document reference
- Implementation file reference
- ✅ Verified status

### 3. Count MUST/SHOULD/MAY Statements

```bash
# Count explicit requirements in Core Specification
grep -o "MUST\|SHOULD\|MAY" docs/SPEC_CORE_V1.md | wc -l
```

**Expected:** 91+ explicit requirement statements

### 4. Verify No Ambiguity

```bash
# Check for TODO/FIXME in specifications
grep -r "TODO\|FIXME\|TBD" docs/*.md

# Should return nothing (exit code 1)
```

---

## Validating Performance Claims

### 1. Run Benchmark Demo

```bash
./build/core/benchmark_demo
```

**Observe:**
- Frame execution times
- Memory allocation patterns
- Deterministic workload execution
- Performance metrics

### 2. Verify Determinism

**Run benchmark twice and compare:**

```bash
./build/core/benchmark_demo > bench1.txt
./build/core/benchmark_demo > bench2.txt
diff bench1.txt bench2.txt
```

**Expected:** No differences (identical output)

### 3. Review Browser Comparison Report

```bash
cat benchmarks/BROWSER_NATIVE_COMPARISON.md
```

**Verify claims:**
- Variance improvement: 10,000x - 24,000x
- P99 latency improvement: 100x - 150x
- Platforms tested documented
- Methodology reproducible
- Raw measurements included

### 4. Run Real-Time Demo

```bash
./build/core/aegis_realtime_demo
```

**Observe:**
- Real-time frame execution
- Consistent frame times
- Low variance

---

## Validating Determinism

### 1. Verify No Banned APIs

```bash
./.github/scripts/check_banned_apis.sh
```

**Expected:** Script should pass with no banned API usage found.

**Script checks for:**
- `rand()`, `random()`, `srand()`
- `time()`, `clock_gettime()`, `gettimeofday()`
- `new`, `delete`, `malloc`, `free`
- OS-specific headers

### 2. Verify IEEE 754 Compliance

```bash
# Check compiler flags in build output
cmake --build build --verbose 2>&1 | grep "fno-fast-math"
cmake --build build --verbose 2>&1 | grep "ffp-contract=off"
```

**Expected:** Both flags should be present.

### 3. Run Determinism Tests

```bash
./build/runtime/conformance_tests/conformance_tests determinism
```

**Expected output:**
```
[1/5] Frame execution is deterministic
  ✓ PASSED

[2/5] Event ordering is deterministic
  ⊘ NOT IMPLEMENTED

[3/5] Memory allocation is deterministic
  ✓ PASSED

[4/5] Floating-point is deterministic
  ✓ PASSED

[5/5] Workload hash is deterministic
  ✓ PASSED
```

**Validation:**
- ✅ All implemented tests pass
- ✅ Floating-point determinism verified
- ✅ Workload hash determinism verified

### 4. Test Workload Replay

```bash
# Run event stream demo with workload
./build/core/event_stream_demo > replay1.txt
./build/core/event_stream_demo > replay2.txt
diff replay1.txt replay2.txt
```

**Expected:** No differences (byte-identical replay)

---

## Validating Documentation

### 1. Check Documentation Completeness

```bash
ls -l docs/
```

**Should contain:**
- ✅ ARCHITECTURE.md
- ✅ CAPABILITY_MODEL.md
- ✅ CORE_FOLDER_CONTRACT.md
- ✅ DETERMINISM.md
- ✅ MEMORY_SYSTEM.md
- ✅ SPEC_CORE_V1.md
- ✅ SPECIFICATION_INDEX.md
- ✅ DEVELOPER_WORKFLOW.md
- ✅ CODING_STANDARDS.md

```bash
ls -l *.md
```

**Should contain:**
- ✅ README.md
- ✅ BUILD.md
- ✅ LICENSE
- ✅ CONTRIBUTING.md (if present)
- ✅ RELEASE_READINESS.md (if present)

### 2. Verify All Links Work

**Manually check links in:**
- README.md
- SPECIFICATION_INDEX.md
- ARCHITECTURE.md

**All internal links should resolve to existing files.**

### 3. Verify Build Instructions

**Follow BUILD.md exactly:**

```bash
# From BUILD.md Quick Start section
git clone https://github.com/sandeep-jaiswar/aegis.git
cd aegis
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Verify output
ls -lh build/core/libaegis_core.a
```

**Should succeed following documented steps.**

### 4. Check for TODOs in Documentation

```bash
grep -r "TODO\|FIXME" docs/*.md *.md
```

**Expected:** No TODOs in frozen specifications or critical documentation.

---

## Common Issues

### Issue: CMake Version Too Old

**Error:**
```
CMake Error: CMake 3.20 or higher is required
```

**Solution:**
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install cmake

# Or download from https://cmake.org/download/
```

### Issue: Compiler Doesn't Support C++23

**Error:**
```
error: C++23 required
```

**Solution:**
- Verify compiler version: `g++ --version` or `clang++ --version`
- Required: GCC 13.0+ or Clang 18.0+
- Upgrade compiler if needed

### Issue: Build Warnings

**Error:**
```
warning: unused variable 'x'
```

**Expected:** Build should have **0 warnings** due to `-Werror` flag.

**If warnings appear:**
- This is a bug in the codebase
- Report as issue on GitHub

### Issue: Test Failures

**Error:**
```
Test Results:
  ✗ Failed:        1
```

**Expected:** All implemented tests should **pass** (0 failures).

**If tests fail:**
- This is a bug in the implementation
- Note which test failed
- Report as issue on GitHub with test output

### Issue: Missing Dependencies

**Error:**
```
fatal error: some_header.hpp: No such file or directory
```

**Solution:**
- Verify clean checkout: `git status`
- Verify all submodules initialized (if any): `git submodule update --init`
- Report as issue if problem persists

---

## Validation Checklist

Use this checklist to verify all claims:

### Build Verification
- [ ] CMake configuration succeeds
- [ ] Build completes with 0 warnings
- [ ] Build completes with 0 errors
- [ ] Core library artifact exists (`build/core/libaegis_core.a`)
- [ ] Test executables built
- [ ] Demo executables built

### Test Verification
- [ ] Conformance tests run: 16/16 passing, 0 failing
- [ ] Capability tests run: 6/6 passing
- [ ] Determinism tests pass
- [ ] Memory tests pass
- [ ] Module loading tests pass

### Specification Verification
- [ ] All specifications frozen at version 1.0.0
- [ ] All core modules have specification coverage
- [ ] 91+ explicit MUST/SHOULD/MAY statements
- [ ] No TODOs in frozen specifications
- [ ] SPECIFICATION_INDEX.md comprehensive

### Determinism Verification
- [ ] No banned APIs (script passes)
- [ ] IEEE 754 flags present (`-fno-fast-math`, `-ffp-contract=off`)
- [ ] Benchmark produces identical output on repeated runs
- [ ] Workload replay is byte-identical
- [ ] Frame execution determinism verified

### Documentation Verification
- [ ] README.md has quick start
- [ ] BUILD.md has complete build instructions
- [ ] All documentation links work
- [ ] ARCHITECTURE.md explains design
- [ ] SPECIFICATION_INDEX.md comprehensive

### Performance Verification
- [ ] Benchmark demo runs successfully
- [ ] Browser comparison report exists
- [ ] Performance claims documented with methodology
- [ ] Raw measurements included

---

## Reporting Issues

If validation fails at any step:

1. **Note the exact error message**
2. **Note your environment:**
   - OS and version
   - Compiler and version
   - CMake version
3. **Reproduce the issue:**
   - Clean build: `rm -rf build && cmake -B build && cmake --build build`
   - Try again
4. **Report on GitHub Issues:**
   - Include error message
   - Include environment details
   - Include steps to reproduce

---

## Contact

**Questions or Issues:**
- GitHub Issues: https://github.com/sandeep-jaiswar/aegis/issues
- GitHub Discussions: https://github.com/sandeep-jaiswar/aegis/discussions

---

## Conclusion

This guide enables independent verification of all Aegis claims:

✅ **Build succeeds** following documented steps  
✅ **Tests pass** with 100% pass rate on implemented tests  
✅ **Specifications are frozen** and comprehensive  
✅ **Determinism is enforced** through tests and tooling  
✅ **Documentation is complete** and accurate  

External reviewers can **confidently validate** all aspects of the Aegis runtime.

---

**End of External Validation Guide**
