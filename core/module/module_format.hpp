#pragma once

#include <cstddef>
#include <cstdint>

namespace aegis::core::module {

// Magic number for .aegis module files (ASCII "AEGS")
constexpr uint32_t MODULE_MAGIC = 0x53474541;

// Module format version
constexpr uint16_t MODULE_VERSION_MAJOR = 1;
constexpr uint16_t MODULE_VERSION_MINOR = 0;

// Section types in the module
enum class section_type : uint32_t {
    metadata = 0x01,      // Module metadata (name, description, etc.)
    code = 0x02,          // Compiled code/bytecode
    assets = 0x03,        // Embedded assets (textures, fonts, etc.)
    capabilities = 0x04,  // Required capabilities
    signature = 0x05,     // Cryptographic signature
    custom = 0xFF         // Custom user-defined sections
};

// Capability flags - declare what features a module requires
enum class capability_flags : uint64_t {
    none = 0x0000,
    gpu_rendering = 0x0001,       // Requires GPU rendering support
    input_events = 0x0002,        // Requires input event handling
    file_io = 0x0004,             // Requires file I/O
    network_io = 0x0008,          // Requires network I/O
    audio = 0x0010,               // Requires audio support
    multithreading = 0x0020,      // Requires multi-threading
    webgpu = 0x0040,              // Requires WebGPU backend
    vulkan = 0x0080,              // Requires Vulkan backend
    custom_shaders = 0x0100,      // Uses custom shader code
    external_resources = 0x0200,  // Loads external resources at runtime
    all = 0xFFFFFFFFFFFFFFFF      // All capabilities
};

// Bitwise operators for capability_flags
inline constexpr capability_flags operator|(capability_flags a, capability_flags b) noexcept {
    return static_cast<capability_flags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

inline constexpr capability_flags operator&(capability_flags a, capability_flags b) noexcept {
    return static_cast<capability_flags>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

inline constexpr bool has_capability(capability_flags flags, capability_flags cap) noexcept {
    return (static_cast<uint64_t>(flags) & static_cast<uint64_t>(cap)) != 0;
}

// Module header - always at the start of the file
struct module_header {
    uint32_t magic{MODULE_MAGIC};          // Magic number: 0x53474541 ("AEGS")
    uint16_t version_major{MODULE_VERSION_MAJOR};
    uint16_t version_minor{MODULE_VERSION_MINOR};
    uint32_t header_size{sizeof(module_header)};
    uint32_t section_count{0};             // Number of sections in the module
    uint32_t _padding1{0};                 // Padding for alignment
    uint64_t total_size{0};                // Total module file size in bytes
    uint64_t module_hash{0};               // FNV-1a hash of module content (excluding this field)
    uint64_t creation_timestamp{0};        // Unix timestamp (seconds since epoch)
    uint64_t capabilities{0};              // Required capabilities (capability_flags)
    uint64_t _padding2{0};                 // Reserved for future use
};

static_assert(sizeof(module_header) == 64, "Module header must be 64 bytes");

// Section header - describes each section in the module
struct section_header {
    section_type type{section_type::metadata};
    uint32_t _padding{0};                  // Padding for alignment
    uint64_t offset{0};                    // Offset from start of file
    uint64_t size{0};                      // Size of section data in bytes
    uint64_t hash{0};                      // FNV-1a hash of section data
    char name[32]{};                       // Human-readable section name (null-terminated)
};

static_assert(sizeof(section_header) == 64, "Section header must be 64 bytes");

// Metadata section structure
struct module_metadata {
    char name[64]{};                       // Module name
    char version[16]{};                    // Module version string
    char author[64]{};                     // Module author
    char description[256]{};               // Module description
    uint64_t _reserved[8]{};               // Reserved for future use
};

static_assert(sizeof(module_metadata) == 464, "Module metadata size");

// Asset entry within assets section
struct asset_entry {
    char name[64]{};                       // Asset name/identifier
    uint32_t type{0};                      // Asset type (application-defined)
    uint32_t _padding{0};                  // Padding for alignment
    uint64_t offset{0};                    // Offset within assets section
    uint64_t size{0};                      // Size of asset data
    uint64_t hash{0};                      // FNV-1a hash of asset data
    uint64_t _reserved[2]{};               // Reserved for future use
};

static_assert(sizeof(asset_entry) == 112, "Asset entry size");

// Module load result
enum class module_load_result : uint8_t {
    success = 0,
    invalid_magic = 1,
    unsupported_version = 2,
    hash_mismatch = 3,
    insufficient_capabilities = 4,
    corrupt_data = 5,
    file_too_small = 6,
    section_out_of_bounds = 7
};

// Version compatibility checking
struct version_info {
    uint16_t major{0};
    uint16_t minor{0};
};

// Check if module version is compatible with runtime version
// Compatible if:
// - Same major version (breaking changes only in major version)
// - Module minor <= runtime minor (runtime supports newer features)
inline constexpr bool is_version_compatible(const version_info& runtime,
                                            const version_info& module) noexcept {
    if (runtime.major != module.major) {
        return false; // Major version mismatch = incompatible
    }
    return module.minor <= runtime.minor; // Module can't require newer features
}

// FNV-1a hash constants
constexpr uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325ULL;
constexpr uint64_t FNV_PRIME = 0x100000001b3ULL;

// Compute FNV-1a hash for module integrity verification
inline uint64_t compute_hash(const uint8_t* data, size_t length) noexcept {
    uint64_t hash = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < length; ++i) {
        hash ^= data[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

} // namespace aegis::core::module
