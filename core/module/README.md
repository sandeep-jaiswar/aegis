# Aegis Module Format (.aegis)

**Status:** Implemented  
**Version:** 1.0.0  
**Last Updated:** 2025-12-15

---

## Overview

The `.aegis` module format is a binary container format for distributable Aegis applications. It enables Aegis to become a true platform by providing:

- **Binary layout**: No text parsing required - modules load directly into memory
- **Versioning rules**: Backward compatibility within major version
- **Signature & integrity**: Hash-based verification ensures module hasn't been tampered with
- **Asset embedding**: Textures, fonts, and other resources embedded directly in the module
- **Capability declaration**: Modules declare what features they need from the runtime

## Acceptance Criteria

✅ **Module loads without parsing text**
- Binary format with direct memory mapping
- All structures are POD (Plain Old Data) types
- No dynamic allocation during header parsing

✅ **Module hash fully defines behavior**
- FNV-1a hash of all module content
- Hash verification ensures bit-identical replay
- Deterministic module generation (same inputs → same output)

✅ **Old modules replay identically on newer runtimes**
- Semantic versioning with compatibility window
- Major version breaks compatibility
- Minor version additions are backward compatible
- Modules can't require features from newer minor versions

---

## File Structure

A `.aegis` module file consists of:

```
┌─────────────────────────────────────┐
│       Module Header (64 bytes)      │  ← Magic, version, hash, capabilities
├─────────────────────────────────────┤
│    Section Headers (64 bytes each)  │  ← Describes each section
├─────────────────────────────────────┤
│         Metadata Section            │  ← Name, version, author, description
├─────────────────────────────────────┤
│          Code Section               │  ← Compiled code/bytecode
├─────────────────────────────────────┤
│         Assets Section              │  ← Embedded resources
├─────────────────────────────────────┤
│      Capabilities Section           │  ← Required runtime features
├─────────────────────────────────────┤
│        Signature Section            │  ← Cryptographic signature (optional)
└─────────────────────────────────────┘
```

### Module Header (64 bytes)

```cpp
struct module_header {
    uint32_t magic;              // 0x53474541 ("AEGS")
    uint16_t version_major;      // Major version
    uint16_t version_minor;      // Minor version
    uint32_t header_size;        // Size of this header
    uint64_t total_size;         // Total file size
    uint64_t module_hash;        // FNV-1a hash for integrity
    uint64_t creation_timestamp; // Unix timestamp
    uint64_t capabilities;       // Required capabilities
    uint32_t section_count;      // Number of sections
    uint32_t _padding;           // Alignment padding
};
```

### Section Header (64 bytes)

```cpp
struct section_header {
    section_type type;      // Section type (metadata, code, assets, etc.)
    uint32_t _padding;      // Alignment
    uint64_t offset;        // Offset from file start
    uint64_t size;          // Section data size
    uint64_t hash;          // FNV-1a hash of section
    char name[32];          // Human-readable name
};
```

---

## Section Types

### 1. Metadata Section (`section_type::metadata`)

Contains module identification and description.

```cpp
struct module_metadata {
    char name[64];          // Module name
    char version[16];       // Version string (e.g., "1.0.0")
    char author[64];        // Author/organization
    char description[256];  // Description
    uint64_t _reserved[8];  // Reserved for future use
};
```

### 2. Code Section (`section_type::code`)

Contains compiled code or bytecode. Format is application-defined. This could be:
- Native machine code
- WASM bytecode
- Custom VM instructions
- Precompiled shaders

### 3. Assets Section (`section_type::assets`)

Contains embedded resources:

```cpp
struct asset_entry {
    char name[64];          // Asset identifier
    uint32_t type;          // Application-defined type
    uint32_t _padding;
    uint64_t offset;        // Offset within assets section
    uint64_t size;          // Asset data size
    uint64_t hash;          // FNV-1a hash of asset
    uint64_t _reserved[2];
};
```

Assets section layout:
```
[ Asset Entry 1 ][ Asset Entry 2 ]...[ Asset Entry N ][ Asset Data... ]
```

### 4. Capabilities Section (`section_type::capabilities`)

Declares required runtime capabilities:

```cpp
enum class capability_flags : uint64_t {
    none = 0x0000,
    gpu_rendering = 0x0001,       // GPU rendering support
    input_events = 0x0002,        // Input event handling
    file_io = 0x0004,             // File I/O
    network_io = 0x0008,          // Network I/O
    audio = 0x0010,               // Audio support
    multithreading = 0x0020,      // Multi-threading
    webgpu = 0x0040,              // WebGPU backend
    vulkan = 0x0080,              // Vulkan backend
    custom_shaders = 0x0100,      // Custom shader code
    external_resources = 0x0200,  // External resources
};
```

### 5. Signature Section (`section_type::signature`)

Optional cryptographic signature for authenticity verification.

---

## Versioning Rules

### Semantic Versioning

Modules use semantic versioning: `MAJOR.MINOR`

**Major version changes:**
- Breaking changes to module format
- Incompatible with previous major versions
- Runtime MUST reject modules with different major version

**Minor version changes:**
- Backward-compatible additions
- New optional sections
- New capability flags
- Runtime with higher minor version can load modules with lower minor version
- Runtime with lower minor version CANNOT load modules with higher minor version

### Compatibility Matrix

| Runtime | Module 1.0 | Module 1.1 | Module 2.0 |
|---------|-----------|-----------|-----------|
| 1.0     | ✅ Yes    | ❌ No     | ❌ No     |
| 1.1     | ✅ Yes    | ✅ Yes    | ❌ No     |
| 2.0     | ❌ No     | ❌ No     | ✅ Yes    |

### Version Compatibility Check

```cpp
bool is_version_compatible(runtime_version, module_version) {
    // Major versions must match exactly
    if (runtime_version.major != module_version.major)
        return false;
    
    // Module minor version must be <= runtime minor version
    return module_version.minor <= runtime_version.minor;
}
```

---

## Integrity & Signatures

### Hash Calculation

Modules use FNV-1a 64-bit hashing for integrity verification:

```cpp
uint64_t compute_hash(const uint8_t* data, size_t length) {
    uint64_t hash = 0xcbf29ce484222325ULL; // FNV offset basis
    for (size_t i = 0; i < length; ++i) {
        hash ^= data[i];
        hash *= 0x100000001b3ULL; // FNV prime
    }
    return hash;
}
```

### Hash Verification

The module hash covers:
- Module header (excluding `module_hash` field itself)
- All section headers
- All section data

Each section also has its own hash for fine-grained verification.

### Loading Process

1. Read module header
2. Verify magic number
3. Check version compatibility
4. Check capability requirements
5. Verify module hash
6. Verify each section hash
7. Extract sections as needed

---

## Capability System

Modules declare required capabilities. Runtime must provide all required capabilities or reject the module.

### Example

```cpp
// Module requires GPU rendering and input events
capability_flags caps = capability_flags::gpu_rendering | 
                       capability_flags::input_events;

// Runtime checks
if (!runtime_has_all_capabilities(caps)) {
    return module_load_result::insufficient_capabilities;
}
```

This prevents:
- Loading GPU-dependent modules on headless systems
- Running network modules in sandboxed environments
- Using file I/O in restricted contexts

---

## Determinism Guarantees

### Same Inputs → Same Output

Given identical inputs:
- Module name, version, author, description
- Code data
- Assets data
- Capabilities
- Creation timestamp

The builder MUST produce byte-identical `.aegis` files.

### Verification

```cpp
// Build module twice with identical inputs
module_builder builder1(buffer1, size);
module_builder builder2(buffer2, size);

// Both initialized with identical data
builder1.init(name, version, author, desc, caps, timestamp);
builder2.init(name, version, author, desc, caps, timestamp);

// Both produce byte-identical output
assert(memcmp(buffer1, buffer2, module_size) == 0);
```

This enables:
- Reproducible builds
- Binary diff for updates
- Audit and compliance
- Verification of module provenance

---

## Usage Examples

### Building a Module

```cpp
// Allocate buffer for module
uint8_t buffer[1024 * 1024];
module_builder builder(buffer, sizeof(buffer));

// Initialize with metadata
builder.init(
    "my_app",                    // name
    "1.0.0",                     // version  
    "My Company",                // author
    "My amazing application",    // description
    capability_flags::gpu_rendering | capability_flags::input_events,
    1702656000                   // timestamp
);

// Add code section
uint8_t code[] = { /* compiled code */ };
builder.add_code(code, sizeof(code));

// Add assets
uint8_t texture[] = { /* texture data */ };
builder.add_asset("logo", ASSET_TYPE_TEXTURE, texture, sizeof(texture));

// Finalize module
builder.finalize();

// Save to file (using platform layer)
save_file("my_app.aegis", builder.get_buffer(), builder.get_module_size());
```

### Loading a Module

```cpp
// Load file (using platform layer)
uint8_t* data = load_file("my_app.aegis", &file_size);

// Create loader
module_loader loader(data, file_size);

// Load and validate
version_info runtime{1, 0};
capability_flags available = capability_flags::all;

if (loader.load(runtime, available) != module_load_result::success) {
    // Handle error
}

// Access metadata
const module_metadata* meta = loader.get_metadata();
printf("Loaded: %s v%s\n", meta->name, meta->version);

// Get code
size_t code_size;
const uint8_t* code = loader.get_code(&code_size);

// Find assets
const asset_entry* logo = loader.find_asset("logo");
const uint8_t* logo_data = loader.get_asset_data(logo);
```

---

## Design Principles

### 1. **No Parsing**

All structures are POD types that can be memory-mapped directly. No string parsing, no JSON/XML processing.

### 2. **Deterministic**

Same inputs produce byte-identical output. Critical for reproducible builds and verification.

### 3. **Self-Describing**

Modules contain all metadata needed to understand their contents. No external manifests required.

### 4. **Verifiable**

Hashes at multiple levels (module, sections, assets) enable fine-grained integrity checking.

### 5. **Extensible**

Custom sections can be added without breaking compatibility. Reserved fields allow future expansion.

### 6. **Minimal**

No compression, no encryption (add as sections if needed). Core format is simple and predictable.

---

## Implementation Notes

### Alignment

All structures are aligned to 8-byte boundaries for efficient memory access.

### Padding

All padding bytes MUST be zero-initialized for deterministic hashing.

### Endianness

All multi-byte values are stored in little-endian format (matches x86/x64/ARM).

### String Handling

All strings are null-terminated and padded with zeros. String fields have fixed sizes.

---

## Security Considerations

### Hash Verification

Always verify hashes before using module data. Hash mismatch indicates corruption or tampering.

### Capability Checking

Always verify runtime provides required capabilities. Missing capabilities can cause crashes or security issues.

### Version Validation

Reject modules with incompatible versions. Version mismatches can cause undefined behavior.

### Bounds Checking

Always validate section offsets and sizes are within file bounds before accessing data.

---

## Future Extensions

Possible future additions (minor version bumps):

- **Compression sections**: Compressed code/assets with compression metadata
- **Encryption sections**: Encrypted sections with key management
- **Dependencies section**: List required external modules
- **Debug information**: Source maps, debug symbols
- **Performance hints**: Optimization metadata for runtime

All additions must maintain backward compatibility.

---

## Acceptance Criteria Verification

### ✅ Module loads without parsing text

```cpp
// Direct memory mapping, no parsing
const module_header* header = (module_header*)data;
if (header->magic == MODULE_MAGIC) { /* valid */ }
```

### ✅ Module hash fully defines behavior

```cpp
// Hash covers all content
uint64_t hash = compute_hash(data, size);
assert(hash == header->module_hash);

// Same inputs produce same hash
assert(build1_hash == build2_hash);
```

### ✅ Old modules replay identically on newer runtimes

```cpp
// v1.0 module loads on v1.1 runtime
assert(is_version_compatible({1,1}, {1,0}) == true);

// Produces identical behavior
assert(execute(v1_0_module_on_v1_0_runtime) == 
       execute(v1_0_module_on_v1_1_runtime));
```

---

## Conclusion

The `.aegis` module format transforms Aegis from a library into a distributable platform. Applications can now be packaged, distributed, and loaded with integrity guarantees, version compatibility, and capability checking.

**This is the moment Aegis becomes distributable.**
