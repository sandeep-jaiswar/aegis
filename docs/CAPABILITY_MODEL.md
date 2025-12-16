# Aegis Capability Model Specification

**Status:** Frozen  
**Version:** 1.0.0  
**Ticket:** MOD-002  
**Last Updated:** 2025-12-16

---

## Overview

The Aegis Capability Model defines a **capability-based security system** that enforces the principle of least privilege for modules. Unlike origin-based security models (e.g., browser same-origin policy), capabilities are explicitly declared and checked at load time, providing deterministic security guarantees.

This specification formalizes:
- Capability declaration format
- Enforcement points in the runtime
- Failure behavior when capabilities are missing
- Comparison to origin-based security models

---

## Core Principles

### 1. No Ambient Authority

**Definition:** A module has NO access to any runtime feature unless it explicitly declares the capability for that feature.

**Enforcement:**
- By default, a module has zero capabilities
- Every capability must be explicitly declared in the module header
- The runtime MUST verify all required capabilities are available before loading
- The runtime MUST NOT provide features beyond what was declared

**Example:**
```cpp
// Module with no capabilities declared
capability_flags caps = capability_flags::none;

// This module CANNOT:
// - Access GPU rendering
// - Handle input events  
// - Perform file I/O
// - Open network connections
// - Use audio features
// - Create threads
```

### 2. Explicit Capability Declaration

**Definition:** All capabilities are declared upfront in the module header using a bitfield of capability flags.

**Format:**
```cpp
enum class capability_flags : uint64_t {
    none = 0x0000,                  // No capabilities
    gpu_rendering = 0x0001,         // GPU rendering support
    input_events = 0x0002,          // Input event handling
    file_io = 0x0004,               // File I/O
    network_io = 0x0008,            // Network I/O
    audio = 0x0010,                 // Audio support
    multithreading = 0x0020,        // Multi-threading
    webgpu = 0x0040,                // WebGPU backend
    vulkan = 0x0080,                // Vulkan backend
    custom_shaders = 0x0100,        // Custom shader code
    external_resources = 0x0200,    // External resources
    all = 0xFFFFFFFFFFFFFFFF        // All capabilities (testing only)
};
```

**Declaration in Module:**
```cpp
// Module declares it needs GPU rendering and input events
capability_flags required = capability_flags::gpu_rendering | 
                           capability_flags::input_events;

module_builder builder(buffer, size);
builder.init("my_app", "1.0.0", "Author", "Description", 
             required,  // ← Capabilities declared here
             timestamp);
```

### 3. Deterministic Failure

**Definition:** When a module requires a capability that the runtime cannot provide, the module load MUST fail with a specific error code.

**Failure Behavior:**
- **Load-time enforcement:** Capabilities are checked during `module_loader::load()`
- **Deterministic result:** Same module + same runtime → same result (success or failure)
- **No partial loading:** Module is either fully loaded with all capabilities, or not loaded at all
- **Clear error codes:** Specific error indicates missing capability

**Error Code:**
```cpp
enum class module_load_result : uint8_t {
    success = 0,
    invalid_magic = 1,
    unsupported_version = 2,
    hash_mismatch = 3,
    insufficient_capabilities = 4,  // ← Capability failure
    corrupt_data = 5,
    file_too_small = 6,
    section_out_of_bounds = 7
};
```

---

## Capability Grammar

### Capability Declaration Syntax

Capabilities are declared using C++ bitwise OR operations on the `capability_flags` enum:

```cpp
// Single capability
capability_flags caps = capability_flags::gpu_rendering;

// Multiple capabilities
capability_flags caps = capability_flags::gpu_rendering | 
                       capability_flags::input_events |
                       capability_flags::audio;

// Check if a capability is present
bool has_gpu = has_capability(caps, capability_flags::gpu_rendering);

// Runtime available capabilities
capability_flags runtime_caps = determine_runtime_capabilities();
```

### Capability Checking

```cpp
// Check if runtime has all required capabilities
capability_flags required = static_cast<capability_flags>(module->capabilities);
capability_flags available = runtime_capabilities;

// Calculate missing capabilities
capability_flags missing = required & ~available;

if (static_cast<uint64_t>(missing) != 0) {
    // Runtime lacks required capabilities
    return module_load_result::insufficient_capabilities;
}
```

### Reserved Capability Space

- **Bits 0-15:** Core runtime capabilities (defined in this spec)
- **Bits 16-31:** Reserved for future Aegis extensions
- **Bits 32-47:** Reserved for vendor-specific capabilities
- **Bits 48-63:** Reserved for experimental features

---

## Enforcement Points

### 1. Module Load Time (Primary Enforcement)

**Location:** `module_loader::load()`

**Process:**
1. Module header is parsed
2. Required capabilities extracted from `module_header::capabilities`
3. Runtime's available capabilities determined
4. Bitwise AND check: `required & ~available`
5. If missing capabilities exist → return `insufficient_capabilities`
6. If all capabilities available → proceed with loading

**Code:**
```cpp
module_load_result load(const version_info& runtime_version,
                       capability_flags available_caps) noexcept {
    // ... version and integrity checks ...
    
    // Check capabilities
    const capability_flags required_caps = 
        static_cast<capability_flags>(header_ptr->capabilities);
    const capability_flags missing_caps = required_caps & ~available_caps;
    
    if (static_cast<uint64_t>(missing_caps) != 0) {
        return module_load_result::insufficient_capabilities;  // ← Enforcement
    }
    
    // ... continue loading ...
}
```

**Determinism Guarantee:**
- Same module + same runtime capabilities → same result
- No race conditions (load is single-threaded)
- No time-of-check-time-of-use issues (capabilities checked once at load)

### 2. Runtime Feature Access (Secondary Enforcement)

**Location:** Individual runtime subsystems (GPU, I/O, audio, etc.)

**Process:**
1. When module attempts to use a feature, runtime checks if capability was declared
2. If capability missing → deterministic failure
3. This is a **defense-in-depth** check; primary enforcement is at load time

**Example (GPU Rendering):**
```cpp
class gpu_renderer {
    void render(const module* mod) {
        // Secondary enforcement: verify module declared gpu_rendering capability
        if (!has_capability(mod->capabilities, capability_flags::gpu_rendering)) {
            // This should never happen if load() worked correctly
            // But provides defense-in-depth
            fail_deterministically("Module missing gpu_rendering capability");
        }
        
        // Proceed with rendering
    }
};
```

### 3. Static Analysis (Development-Time Enforcement)

**Location:** Build tools and linters

**Process:**
- Static analyzer scans module code for API usage
- Warns if module uses APIs without declaring corresponding capabilities
- Example: Module calls file I/O functions without `file_io` capability

**Example Warning:**
```
Warning: Module uses FileAPI but does not declare 'file_io' capability
  File: src/app.cpp:42
  API: open_file("data.txt")
  Required: capability_flags::file_io
```

---

## Failure Behavior

### Load Failure

**Trigger:** Runtime lacks capabilities required by module

**Behavior:**
1. `module_loader::load()` returns `module_load_result::insufficient_capabilities`
2. Module is NOT loaded into memory
3. No partial execution
4. No side effects (no files created, no network connections, etc.)
5. Deterministic error message can be generated

**Example:**
```cpp
module_loader loader(data, size);
version_info runtime{1, 0};

// Runtime only supports GPU rendering, no input events
capability_flags available = capability_flags::gpu_rendering;

module_load_result result = loader.load(runtime, available);

if (result == module_load_result::insufficient_capabilities) {
    // Module requires capabilities not available
    // Can inspect module header to determine which ones:
    const module_header& header = loader.get_header();
    capability_flags required = static_cast<capability_flags>(header.capabilities);
    capability_flags missing = required & ~available;
    
    // Log missing capabilities
    if (has_capability(missing, capability_flags::input_events)) {
        log("Missing capability: input_events");
    }
    // ... etc
}
```

### Runtime Feature Access Failure

**Trigger:** Module attempts to use feature without declaring capability

**Behavior:**
1. Deterministic failure (assertion, error code, or controlled crash)
2. No undefined behavior
3. No security breach (feature is NOT provided)
4. Logged for debugging

**Example:**
```cpp
// Module did not declare file_io capability
if (!has_capability(module->capabilities, capability_flags::file_io)) {
    return error::capability_not_declared;  // Deterministic failure
}
```

### Capability Upgrade (Not Allowed)

**Trigger:** Module attempts to request additional capabilities at runtime

**Behavior:**
- **This is explicitly forbidden**
- Capabilities are immutable after module load
- No runtime capability escalation
- No dynamic permission requests

**Rationale:**
- Ensures deterministic security model
- Prevents confused deputy attacks
- Makes security auditing simple (inspect module header only)

---

## Comparison to Origin-Based Security

### Origin-Based Security (e.g., Browser Same-Origin Policy)

**Model:**
- Security boundary is the "origin" (protocol + domain + port)
- Resources from same origin have full access to each other
- Resources from different origins are isolated (unless CORS relaxes it)
- Ambient authority: Scripts from trusted origin have broad access
- Runtime permission requests (e.g., geolocation, camera)

**Characteristics:**
- **Ambient authority:** Same-origin scripts have implicit access
- **Dynamic permissions:** Can request permissions at runtime
- **Complex model:** CORS, CSP, sandboxing add complexity
- **Time-of-check-time-of-use:** Permission can change during execution

**Example:**
```javascript
// Browser: Script from https://example.com can access:
fetch('https://example.com/api/data');      // ✓ Same origin
fetch('https://other.com/api/data');        // ✗ Different origin (unless CORS)
navigator.geolocation.getCurrentPosition(); // ? Prompt user at runtime
```

### Capability-Based Security (Aegis Model)

**Model:**
- Security boundary is the explicit capability set
- Module has ZERO access by default
- All capabilities declared upfront in module header
- No ambient authority
- No runtime permission requests

**Characteristics:**
- **Zero ambient authority:** Module starts with nothing
- **Static permissions:** All capabilities declared at build time
- **Simple model:** Bitfield check, deterministic
- **No TOCTOU:** Capabilities checked once at load, then immutable

**Example:**
```cpp
// Aegis: Module with gpu_rendering + input_events
capability_flags caps = capability_flags::gpu_rendering | 
                       capability_flags::input_events;

// Can use:
renderer.draw();           // ✓ Has gpu_rendering
input.handle_event();      // ✓ Has input_events

// Cannot use:
file.open("data.txt");     // ✗ No file_io capability
network.connect("url");    // ✗ No network_io capability
```

### Key Differences

| Aspect | Origin-Based | Capability-Based (Aegis) |
|--------|-------------|--------------------------|
| **Default Access** | Ambient (same-origin has access) | None (zero by default) |
| **Permission Requests** | Runtime (prompt user) | Build-time (static declaration) |
| **Complexity** | High (CORS, CSP, sandboxing) | Low (bitfield check) |
| **Determinism** | Non-deterministic (user prompts) | Deterministic (fixed at load) |
| **Security Audit** | Complex (analyze runtime behavior) | Simple (inspect module header) |
| **Granularity** | Origin-level | Feature-level |
| **Evolution** | Complex (CORS policies evolve) | Simple (add new capability flags) |

### Advantages of Capability-Based Model

1. **Determinism:** Same inputs → same security outcome
2. **Simplicity:** No complex policies (CORS, CSP)
3. **Auditability:** Security posture visible in module header
4. **Least Privilege:** Module gets exactly what it declares, nothing more
5. **No Confused Deputy:** No ambient authority to exploit
6. **Replayability:** Security decisions are deterministic and replayable

### Trade-offs

| Origin-Based | Capability-Based |
|--------------|------------------|
| ✓ Flexible runtime permissions | ✗ No runtime permission escalation |
| ✓ User can grant/revoke at runtime | ✗ Capabilities fixed at build time |
| ✗ Complex security model | ✓ Simple security model |
| ✗ Non-deterministic | ✓ Deterministic |
| ✗ Ambient authority risks | ✓ Zero ambient authority |

---

## Capability Testability and Replayability

### Testable Properties

1. **Module with capability X loads successfully on runtime with X**
   ```cpp
   capability_flags caps = capability_flags::gpu_rendering;
   assert(loader.load(runtime, caps) == module_load_result::success);
   ```

2. **Module with capability X fails on runtime without X**
   ```cpp
   capability_flags caps = capability_flags::none;
   assert(loader.load(runtime, caps) == module_load_result::insufficient_capabilities);
   ```

3. **Module with capabilities X|Y requires both**
   ```cpp
   capability_flags required = capability_flags::gpu_rendering | capability_flags::input_events;
   capability_flags available = capability_flags::gpu_rendering; // Missing input_events
   assert(loader.load(runtime, available) == module_load_result::insufficient_capabilities);
   ```

4. **Capability check is deterministic**
   ```cpp
   // Same module, same runtime → same result
   module_load_result result1 = loader.load(runtime, available);
   module_load_result result2 = loader.load(runtime, available);
   assert(result1 == result2);  // Deterministic
   ```

### Replay Testing

Capability enforcement is fully replayable:

```cpp
// Record: Module load attempt with specific capabilities
struct load_record {
    uint64_t module_hash;         // Module identity
    capability_flags available;   // Runtime capabilities
    module_load_result result;    // Load result
};

// Replay: Load same module with same capabilities
load_record record = load_from_log();
module_loader loader(module_data, size);
module_load_result replay_result = loader.load(runtime, record.available);

// Result MUST be identical
assert(replay_result == record.result);
```

**Replay Guarantees:**
- Same module hash + same runtime capabilities → same load result
- No time dependencies
- No external dependencies
- No non-deterministic behavior

### Test Matrix

| Module Capabilities | Runtime Capabilities | Expected Result |
|---------------------|---------------------|-----------------|
| `none` | `none` | ✓ Success |
| `none` | `gpu_rendering` | ✓ Success (runtime has more) |
| `gpu_rendering` | `none` | ✗ `insufficient_capabilities` |
| `gpu_rendering` | `gpu_rendering` | ✓ Success |
| `gpu_rendering` | `input_events` | ✗ `insufficient_capabilities` |
| `gpu_rendering \| input_events` | `gpu_rendering` | ✗ `insufficient_capabilities` (missing input_events) |
| `gpu_rendering \| input_events` | `gpu_rendering \| input_events` | ✓ Success |
| `gpu_rendering \| input_events` | `all` | ✓ Success (runtime has more) |

---

## Capability Evolution

### Adding New Capabilities (Minor Version)

New capabilities can be added in minor version updates:

```cpp
// Version 1.0
enum class capability_flags : uint64_t {
    gpu_rendering = 0x0001,
    input_events = 0x0002,
    // ...
};

// Version 1.1 adds new capability
enum class capability_flags : uint64_t {
    gpu_rendering = 0x0001,
    input_events = 0x0002,
    // ... existing capabilities ...
    new_feature = 0x0400,  // ← New in 1.1
};
```

**Backward Compatibility:**
- Modules built with v1.0 don't use new capability → load fine on v1.1 runtime
- Modules built with v1.1 using new capability → fail on v1.0 runtime (deterministic failure)

**Version Check:**
```cpp
// Runtime 1.0 loading module 1.1
if (module.version.minor > runtime.version.minor) {
    return module_load_result::unsupported_version;
}
```

### Removing Capabilities (Major Version)

Removing capabilities requires major version bump:

```cpp
// Version 2.0 removes deprecated capability
enum class capability_flags : uint64_t {
    gpu_rendering = 0x0001,
    input_events = 0x0002,
    // removed: deprecated_feature = 0x0800  ← Removed in 2.0
};
```

**Breaking Change:**
- Modules using removed capability cannot load on v2.0
- Major version mismatch → deterministic failure
- Migration path: Rebuild module for v2.0 without removed capability

---

## Security Considerations

### Threat Model

**Threats Mitigated:**
1. **Privilege Escalation:** Module cannot access features without declaring them
2. **Confused Deputy:** No ambient authority to exploit
3. **Supply Chain Attacks:** Module capabilities visible in header (easy audit)
4. **Side-Channel Attacks:** Deterministic failures (no timing variations)

**Threats NOT Mitigated:**
1. **Bugs in Runtime:** If runtime has vulnerability, capabilities don't help
2. **Social Engineering:** User might run malicious module with declared capabilities
3. **Physical Access:** Attacker with physical access can bypass software controls

### Best Practices

1. **Principle of Least Privilege:** Declare only capabilities actually needed
2. **Capability Auditing:** Review module headers before deployment
3. **Sandboxing:** Combine capability model with OS-level sandboxing
4. **Static Analysis:** Use tools to verify module doesn't use undeclared APIs

### Vulnerability Disclosure

If capability enforcement is bypassed:
1. **Critical Security Bug:** Module can access features without declaring them
2. **Immediate Fix Required:** Violates core security model
3. **Disclosure Process:** Follow responsible disclosure

---

## Implementation Notes

### Capability Determination

Runtime must determine available capabilities at startup:

```cpp
capability_flags determine_runtime_capabilities() {
    capability_flags caps = capability_flags::none;
    
    // Check GPU availability
    if (has_gpu_support()) {
        caps = caps | capability_flags::gpu_rendering;
    }
    
    // Check input system
    if (has_input_system()) {
        caps = caps | capability_flags::input_events;
    }
    
    // Check file I/O
    if (can_access_filesystem()) {
        caps = caps | capability_flags::file_io;
    }
    
    // ... etc
    
    return caps;
}
```

### Capability Logging

For debugging and auditing:

```cpp
void log_capability_check(const module_header& header, 
                         capability_flags available) {
    capability_flags required = static_cast<capability_flags>(header.capabilities);
    capability_flags missing = required & ~available;
    
    if (static_cast<uint64_t>(missing) != 0) {
        log_error("Module requires capabilities not available:");
        if (has_capability(missing, capability_flags::gpu_rendering)) {
            log_error("  - gpu_rendering");
        }
        if (has_capability(missing, capability_flags::input_events)) {
            log_error("  - input_events");
        }
        // ... etc
    }
}
```

---

## Acceptance Criteria Verification

### ✅ No Ambient Authority Exists

**Verified by:**
- Default capability set is `capability_flags::none`
- Module must explicitly declare every capability
- Runtime provides ZERO features without declaration

**Code Evidence:**
```cpp
// Module with no capabilities declared
capability_flags caps = capability_flags::none;
builder.init("app", "1.0.0", "Author", "Desc", caps, timestamp);

// This module can use NOTHING
// No GPU, no input, no I/O, no network, no audio, etc.
```

### ✅ Missing Capability → Deterministic Failure

**Verified by:**
- Load-time capability check in `module_loader::load()`
- Returns `module_load_result::insufficient_capabilities`
- Same module + same runtime → same result

**Code Evidence:**
```cpp
const capability_flags required_caps = 
    static_cast<capability_flags>(header_ptr->capabilities);
const capability_flags missing_caps = required_caps & ~available_caps;

if (static_cast<uint64_t>(missing_caps) != 0) {
    return module_load_result::insufficient_capabilities;  // Deterministic failure
}
```

### ✅ Capability Enforcement Testable and Replayable

**Verified by:**
- Deterministic capability checking algorithm
- No time dependencies, no external state
- Replay: same inputs → same outputs

**Test Evidence:**
```cpp
// Test: Module with capability X fails on runtime without X
capability_flags required = capability_flags::gpu_rendering;
capability_flags available = capability_flags::none;

module_load_result result = loader.load(runtime, available);
assert(result == module_load_result::insufficient_capabilities);

// Replay produces identical result
module_load_result replay_result = loader.load(runtime, available);
assert(replay_result == result);  // Deterministic
```

---

## Conclusion

The Aegis Capability Model provides a **simple, deterministic, and auditable** security model based on explicit capability declarations. Unlike origin-based security, it eliminates ambient authority and ensures deterministic failure when capabilities are missing.

**Key Properties:**
- ✅ Zero ambient authority
- ✅ Deterministic failure behavior
- ✅ Fully testable and replayable
- ✅ Simple bitfield-based enforcement
- ✅ No runtime permission escalation
- ✅ Easy security auditing (inspect module header)

This model aligns with Aegis's core principles: **determinism, control, and simplicity**.

---

**End of Capability Model Specification**
