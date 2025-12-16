# Aegis Specification Index

**Version:** 1.0.0  
**Status:** Frozen  
**Last Updated:** 2025-12-16

---

## Overview

This document provides a comprehensive index of all Aegis specifications, organized by topic and requirement. Use this as your starting point for understanding the Aegis runtime architecture and implementation contracts.

---

## Primary Specifications

### 1. Core Runtime Specification
**Document:** [SPEC_CORE_V1.md](SPEC_CORE_V1.md)  
**Ticket:** SPEC-001  
**Status:** ✅ Frozen

**Covers:**
- Frame lifecycle semantics (7 phases)
- Memory model (arena/frame/pool allocators)
- State transition rules
- Event ordering rules
- Layout engine contract
- Scene graph contract
- Benchmark system contract
- Undefined behavior list
- Error handling
- Compilation requirements

**Key Sections:**
- §3: Frame Lifecycle Contract - 7-phase execution model
- §4: Memory Management Contract - Arena, Frame, Pool allocators
- §5: State Management Contract - Immutable state principles
- §6: Event System Contract - POD events with deterministic ordering
- §7: Layout Engine Contract - One-pass deterministic layout
- §8: Scene Graph Contract - Stable node IDs and structural diffs
- §9: Benchmark System Contract - Deterministic performance measurement
- §12: Undefined Behavior - Explicit list of forbidden patterns
- §13: Acceptance Criteria - Verification checklist

**Acceptance Criteria:**
- ✅ Spec enables clean-room reimplementation
- ✅ No behavior depends on "current implementation"
- ✅ All MUST/SHOULD/MAY statements explicit (91+ statements)
- ✅ Reviewed against existing codebase for mismatches

---

### 2. Determinism Contract
**Document:** [DETERMINISM.md](DETERMINISM.md)  
**Ticket:** SPEC-002  
**Status:** ✅ Frozen

**Covers:**
- Fundamental determinism guarantee
- Explicit inputs vs. implicit state
- Deterministic data structures
- Floating-point determinism (IEEE 754)
- Memory determinism
- Event determinism and ordering
- GPU determinism boundaries
- Randomness exclusions
- Testing and verification

**Key Sections:**
- §2: Fundamental Determinism Guarantee - Byte-identical outputs
- §4: Floating-Point Determinism - IEEE 754 compliance
- §5: Memory Determinism - Allocation patterns and pointer handling
- §6: Event Determinism - Timestamp ordering and replay
- §8: Randomness and Non-Determinism - Forbidden sources
- §9: Testing Determinism - Replay testing and workload hashing
- §12.3: GPU Determinism Boundaries - Core vs. Runtime responsibilities

**Acceptance Criteria:**
- ✅ Given module + event log → identical output
- ✅ Replay tests enforce contract
- ✅ Non-deterministic APIs explicitly banned
- ✅ GPU determinism boundaries specified

---

### 3. Memory Safety & Lifetime Specification
**Document:** [MEMORY_SYSTEM.md](MEMORY_SYSTEM.md)  
**Ticket:** SPEC-003  
**Status:** ✅ Frozen

**Covers:**
- Frame allocator semantics
- Arena lifetime rules
- Pool allocator semantics
- Structural sharing guarantees
- Forbidden allocation patterns
- Memory behavior reproducibility
- Cross-platform memory reproducibility

**Key Sections:**
- Arena Allocator - O(1) bulk allocation with reset
- Frame Allocator - Per-frame linear allocation
- Pool Allocator - Fixed-size object allocation
- Structural Sharing Guarantees - Safe immutable sharing
- Forbidden Allocation Patterns - 7 explicit anti-patterns
- Memory Behavior Reproducibility Under Replay

**Acceptance Criteria:**
- ✅ No ambiguity around object lifetime
- ✅ All allocators have documented invariants
- ✅ Memory behavior reproducible under replay
- ✅ Structural sharing rules explicit
- ✅ Forbidden patterns documented

---

### 4. Module Format Specification (.aegis Binary Format)
**Document:** [core/module/README.md](../core/module/README.md)  
**Ticket:** MOD-001  
**Status:** ✅ Frozen

**Covers:**
- Binary layout and file structure
- Versioning rules and compatibility
- Integrity hashing (FNV-1a)
- Asset embedding
- Section types (metadata, code, assets, capabilities, signature)
- Deterministic module generation

**Key Sections:**
- Module Header - Magic number, version, hash, capabilities
- Section Headers - Offset, size, hash for each section
- Metadata Section - Name, version, author, description
- Code Section - Compiled code/bytecode
- Assets Section - Embedded resources with metadata
- Versioning Rules - Semantic versioning and compatibility matrix
- Integrity Verification - Hash calculation and verification

**Acceptance Criteria:**
- ✅ No runtime parsing of text
- ✅ Module hash fully defines behavior
- ✅ Old modules replay correctly on newer runtimes (within version contract)
- ✅ Binary format with direct memory mapping
- ✅ Deterministic module generation (same inputs → same output)

---

### 5. Capability Model Specification
**Document:** [CAPABILITY_MODEL.md](CAPABILITY_MODEL.md)  
**Ticket:** MOD-002  
**Status:** ✅ Frozen

**Covers:**
- Capability declaration format
- Enforcement points in runtime
- Failure behavior when capabilities missing
- Comparison to origin-based security
- No ambient authority principle
- Deterministic capability checking

**Key Sections:**
- Core Principles - No ambient authority, explicit declaration, deterministic failure
- Capability Grammar - Bitfield syntax and checking
- Enforcement Points - Load-time and runtime checks
- Failure Behavior - Deterministic error codes
- Comparison to Origin-Based Security - Advantages and trade-offs
- Capability Testability - Test matrix and replay guarantees
- Security Considerations - Threat model and best practices

**Acceptance Criteria:**
- ✅ No ambient authority exists
- ✅ Missing capability → deterministic failure
- ✅ Capability enforcement testable and replayable
- ✅ Zero default permissions
- ✅ Load-time enforcement with clear error codes

---

## Supporting Documents

### Architecture Documents

#### ARCHITECTURE.md
**Purpose:** High-level system architecture and design principles  
**Covers:**
- Architectural goals (determinism, control, scaling)
- Layer responsibilities (platform, rendering, scene, state)
- Frame lifecycle
- Memory model
- Concurrency model
- Security model

**Key Insight:** "Aegis is an engine, not a framework"

#### CORE_FOLDER_CONTRACT.md
**Purpose:** Hard dependency boundaries for core/  
**Covers:**
- Core folder rules (no OS dependencies)
- Module contracts (memory, state, events, layout, scene, diff, math)
- Runtime contract
- Enforcement rules

**Key Rule:** "Nothing in `core/` may depend on anything outside `core/`"

---

### Development Guides

#### CODING_STANDARDS.md
**Purpose:** Code style and quality guidelines  
**Covers:**
- Code formatting
- Naming conventions
- Documentation requirements
- Error handling
- Testing standards

#### DEVELOPER_WORKFLOW.md
**Purpose:** Development process and tools  
**Covers:**
- Build system
- Testing workflow
- Benchmarking
- Profiling
- Debugging

---

## Specification Cross-Reference

### By Topic

#### Frame Execution
- **Primary:** SPEC_CORE_V1.md §3 (Frame Lifecycle)
- **Determinism:** DETERMINISM.md §6 (Event Determinism)
- **Architecture:** ARCHITECTURE.md §4 (Frame Lifecycle)
- **Implementation:** `core/frame/frame_lifecycle.{hpp,cpp}`

#### Memory Management
- **Primary:** SPEC_CORE_V1.md §4 (Memory Management)
- **Details:** MEMORY_SYSTEM.md (all sections)
- **Determinism:** DETERMINISM.md §5 (Memory Determinism)
- **Implementation:** `core/memory/{arena,frame,pool}_allocator.{hpp,cpp}`

#### State Management
- **Primary:** SPEC_CORE_V1.md §5 (State Management)
- **Determinism:** DETERMINISM.md §2 (Core Invariant)
- **Architecture:** ARCHITECTURE.md §3.5 (State Engine)
- **Implementation:** `core/state/*.{hpp,cpp}`

#### Event System
- **Primary:** SPEC_CORE_V1.md §6 (Event System)
- **Determinism:** DETERMINISM.md §6 (Event Determinism)
- **Contract:** CORE_FOLDER_CONTRACT.md §3 (core/events/)
- **Implementation:** `core/events/*.{hpp,cpp}`

#### Layout Computation
- **Primary:** SPEC_CORE_V1.md §7 (Layout Engine)
- **Determinism:** DETERMINISM.md §4 (Floating-Point)
- **Contract:** CORE_FOLDER_CONTRACT.md §4 (core/layout/)
- **Implementation:** `core/layout/*.{hpp,cpp}`

#### Scene Graph
- **Primary:** SPEC_CORE_V1.md §8 (Scene Graph)
- **Determinism:** DETERMINISM.md §5 (Memory Determinism)
- **Architecture:** ARCHITECTURE.md §3.3 (Scene Graph)
- **Implementation:** `core/frame/scene_graph.{hpp,cpp}`

#### GPU Operations
- **Primary:** SPEC_CORE_V1.md §8.2 (Scene Diff)
- **Determinism:** DETERMINISM.md §12.3 (GPU Determinism Boundaries)
- **Architecture:** ARCHITECTURE.md §3.2 (Rendering Engine)
- **Implementation:** `core/frame/gpu_command*.{hpp,cpp}`

#### Module Distribution
- **Primary:** core/module/README.md (Module Format)
- **Security:** CAPABILITY_MODEL.md (Capability System)
- **Determinism:** DETERMINISM.md §2 (Core Invariant)
- **Implementation:** `core/module/{module_format,module_builder,module_loader}.hpp`

#### Capability-Based Security
- **Primary:** CAPABILITY_MODEL.md (all sections)
- **Format:** core/module/README.md §4 (Capabilities Section)
- **Implementation:** `core/module/module_format.hpp` (capability_flags)

---

## Requirement Mapping

### SPEC-001: Aegis Core Specification v1.0

| Requirement | Document | Section | Status |
|-------------|----------|---------|--------|
| Frame lifecycle semantics | SPEC_CORE_V1.md | §3 | ✅ Complete |
| Determinism guarantees | DETERMINISM.md | §2 | ✅ Complete |
| State transition rules | SPEC_CORE_V1.md | §5 | ✅ Complete |
| Memory model (arena/frame/pool) | SPEC_CORE_V1.md | §4 | ✅ Complete |
| Event ordering rules | SPEC_CORE_V1.md | §6 | ✅ Complete |
| Undefined behavior list | SPEC_CORE_V1.md | §12 | ✅ Complete |

### SPEC-002: Determinism Contract

| Requirement | Document | Section | Status |
|-------------|----------|---------|--------|
| Inputs considered part of determinism | DETERMINISM.md | §3 | ✅ Complete |
| Explicit exclusions (OS, clocks, entropy) | DETERMINISM.md | §8 | ✅ Complete |
| Floating-point rules | DETERMINISM.md | §4 | ✅ Complete |
| GPU determinism boundaries | DETERMINISM.md | §12.3 | ✅ Complete |
| Given module + event log → identical output | DETERMINISM.md | §2.1 | ✅ Complete |
| Replay tests enforce contract | DETERMINISM.md | §9 | ✅ Complete |
| Non-deterministic APIs explicitly banned | DETERMINISM.md | §8.1 | ✅ Complete |

### SPEC-003: Memory Safety & Lifetime Spec

| Requirement | Document | Section | Status |
|-------------|----------|---------|--------|
| Frame allocator semantics | MEMORY_SYSTEM.md | Frame Allocator | ✅ Complete |
| Arena lifetime rules | MEMORY_SYSTEM.md | Arena Allocator | ✅ Complete |
| Structural sharing guarantees | MEMORY_SYSTEM.md | Structural Sharing | ✅ Complete |
| Forbidden allocation patterns | MEMORY_SYSTEM.md | Forbidden Patterns | ✅ Complete |
| No ambiguity around object lifetime | MEMORY_SYSTEM.md | Lifetime Rules | ✅ Complete |
| All allocators have documented invariants | SPEC_CORE_V1.md | §4 | ✅ Complete |
| Memory behavior reproducible under replay | MEMORY_SYSTEM.md | Reproducibility | ✅ Complete |

### MOD-001: .aegis Binary Format Specification

| Requirement | Document | Section | Status |
|-------------|----------|---------|--------|
| Binary layout | core/module/README.md | File Structure | ✅ Complete |
| Versioning | core/module/README.md | Versioning Rules | ✅ Complete |
| Integrity hashing | core/module/README.md | Integrity & Signatures | ✅ Complete |
| Capability declarations | core/module/README.md | Capability System | ✅ Complete |
| Asset embedding | core/module/README.md | Assets Section | ✅ Complete |
| No runtime parsing of text | core/module/README.md | Design Principles | ✅ Complete |
| Module hash fully defines behavior | core/module/README.md | Hash Calculation | ✅ Complete |
| Old modules replay on newer runtimes | core/module/README.md | Compatibility Matrix | ✅ Complete |

### MOD-002: Capability Model Specification

| Requirement | Document | Section | Status |
|-------------|----------|---------|--------|
| Capability declaration format | CAPABILITY_MODEL.md | Capability Grammar | ✅ Complete |
| Enforcement points | CAPABILITY_MODEL.md | Enforcement Points | ✅ Complete |
| Failure behavior | CAPABILITY_MODEL.md | Failure Behavior | ✅ Complete |
| Comparison to origin-based security | CAPABILITY_MODEL.md | Comparison | ✅ Complete |
| No ambient authority exists | CAPABILITY_MODEL.md | Core Principles §1 | ✅ Complete |
| Missing capability → deterministic failure | CAPABILITY_MODEL.md | Core Principles §3 | ✅ Complete |
| Capability enforcement testable | CAPABILITY_MODEL.md | Testability | ✅ Complete |
| Capability enforcement replayable | CAPABILITY_MODEL.md | Replay Testing | ✅ Complete |

---
| All allocators have documented invariants | SPEC_CORE_V1.md | §4 | ✅ Complete |
| Memory behavior reproducible under replay | MEMORY_SYSTEM.md | Reproducibility | ✅ Complete |

---

## Implementation Verification

### Core Module Mapping

| Core Module | Specification | Implementation | Verified |
|-------------|---------------|----------------|----------|
| `core/memory/` | SPEC_CORE_V1.md §4, MEMORY_SYSTEM.md | `{arena,frame,pool}_allocator.{hpp,cpp}` | ✅ |
| `core/events/` | SPEC_CORE_V1.md §6 | `input_event.{hpp,cpp}` | ✅ |
| `core/state/` | SPEC_CORE_V1.md §5 | `*.{hpp,cpp}` | ✅ |
| `core/layout/` | SPEC_CORE_V1.md §7 | `*.{hpp,cpp}` | ✅ |
| `core/frame/` | SPEC_CORE_V1.md §3, §8 | `frame_lifecycle.{hpp,cpp}`, `scene_graph.{hpp,cpp}` | ✅ |
| `core/benchmark/` | SPEC_CORE_V1.md §9 | `benchmark*.{hpp,cpp}` | ✅ |
| `core/module/` | core/module/README.md, CAPABILITY_MODEL.md | `module_{format,builder,loader}.hpp`, `demo_module.cpp` | ✅ |

---

## Using This Index

### For New Developers

1. Start with [ARCHITECTURE.md](ARCHITECTURE.md) for high-level understanding
2. Read [CORE_FOLDER_CONTRACT.md](CORE_FOLDER_CONTRACT.md) for dependency rules
3. Study [SPEC_CORE_V1.md](SPEC_CORE_V1.md) for detailed contracts
4. Reference [DETERMINISM.md](DETERMINISM.md) when implementing features
5. Check [MEMORY_SYSTEM.md](MEMORY_SYSTEM.md) for memory management
6. Review [core/module/README.md](../core/module/README.md) for module distribution
7. Study [CAPABILITY_MODEL.md](CAPABILITY_MODEL.md) for security model

### For Implementers

1. Start with the relevant section in [SPEC_CORE_V1.md](SPEC_CORE_V1.md)
2. Cross-reference with [DETERMINISM.md](DETERMINISM.md) for determinism requirements
3. Check [MEMORY_SYSTEM.md](MEMORY_SYSTEM.md) if dealing with allocations
4. Review [CAPABILITY_MODEL.md](CAPABILITY_MODEL.md) for security requirements
5. Verify against acceptance criteria in each document
6. Run verification checklist from §13.1 in SPEC_CORE_V1.md

### For Reviewers

1. Check implementation against [SPEC_CORE_V1.md](SPEC_CORE_V1.md) contracts
2. Verify determinism requirements from [DETERMINISM.md](DETERMINISM.md)
3. Ensure no forbidden patterns from [MEMORY_SYSTEM.md](MEMORY_SYSTEM.md)
4. Verify no violations of [CORE_FOLDER_CONTRACT.md](CORE_FOLDER_CONTRACT.md)
5. Check capability declarations in [CAPABILITY_MODEL.md](CAPABILITY_MODEL.md)
6. Check all MUST requirements are satisfied

---

## Version Compatibility

All specifications are version 1.0.0 and frozen as of 2025-12-16.

| Document | Version | Status | Last Updated |
|----------|---------|--------|--------------|
| SPEC_CORE_V1.md | 1.0.0 | Frozen | 2025-12-16 |
| DETERMINISM.md | 1.0.0 | Frozen | 2025-12-16 |
| MEMORY_SYSTEM.md | 1.0.0 | Frozen | 2025-12-16 |
| core/module/README.md | 1.0.0 | Frozen | 2025-12-16 |
| CAPABILITY_MODEL.md | 1.0.0 | Frozen | 2025-12-16 |
| ARCHITECTURE.md | 1.0.0 | Stable | - |
| CORE_FOLDER_CONTRACT.md | 1.0.0 | Stable | - |

**Frozen** means the specification is production-ready and changes require major version bump.  
**Stable** means the document is mature but may have minor clarifications.

---

## Glossary

### Key Terms

- **Determinism**: Same inputs produce byte-identical outputs
- **Frame**: One complete execution cycle (7 phases)
- **Arena Allocator**: Bulk allocator with O(1) reset
- **Frame Allocator**: Per-frame allocator, resets at end_frame()
- **Pool Allocator**: Fixed-size object allocator with free list
- **POD Type**: Plain Old Data - trivial, standard layout type
- **Immutable State**: State that never changes after creation
- **Structural Sharing**: Sharing immutable data between states
- **Scene Graph**: Retained-mode UI representation
- **Scene Diff**: Minimal change set between two scene graphs
- **Workload**: Recorded event sequence for replay
- **Conforming Implementation**: Implementation satisfying all MUST requirements
- **Module**: Distributable binary package (.aegis file)
- **Capability**: Permission to use a specific runtime feature
- **Ambient Authority**: Implicit access without explicit permission (forbidden in Aegis)
- **Load-time Enforcement**: Security checks performed when loading a module

### Abbreviations

- **MUST**: Required for conformance
- **SHOULD**: Recommended but not required  
- **MAY**: Optional
- **POD**: Plain Old Data
- **IEEE 754**: Floating-point standard
- **FNV-1a**: Fowler-Noll-Vo hash algorithm
- **O(1)**: Constant time complexity
- **O(n)**: Linear time complexity
- **CORS**: Cross-Origin Resource Sharing (browser security)
- **CSP**: Content Security Policy (browser security)

---

## Contact and Updates

For questions or clarifications about specifications:

1. Check the specific document's reference section
2. Review related documents in cross-reference
3. Check implementation in `core/` modules
4. Refer to acceptance criteria and verification checklist

**Note:** Specifications are frozen. Changes require following the migration and evolution rules defined in each document.

---

**End of Specification Index**
