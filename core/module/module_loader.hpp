#pragma once

#include "module_format.hpp"

#include <cstring>

namespace aegis::core::module {

// Module loader - loads and validates .aegis module files
// Performs integrity checking and version validation
class module_loader {
  public:
    // Create loader with module data buffer
    explicit module_loader(const uint8_t* module_data_ptr, size_t data_size_val) noexcept
        : data(module_data_ptr), data_size(data_size_val) {
    }

    ~module_loader() noexcept = default;

    // Disable copy and move
    module_loader(const module_loader&) = delete;
    module_loader& operator=(const module_loader&) = delete;
    module_loader(module_loader&&) = delete;
    module_loader& operator=(module_loader&&) = delete;

    // Load and validate the module
    // runtime_version: version of the runtime loading the module
    // available_caps: capabilities available in the runtime
    [[nodiscard]] module_load_result load(const version_info& runtime_version,
                                          capability_flags available_caps) noexcept {
        // Check minimum file size
        if (data_size < sizeof(module_header)) {
            return module_load_result::file_too_small;
        }

        // Read and validate header
        const module_header* header_ptr = reinterpret_cast<const module_header*>(data);
        
        // Validate magic number
        if (header_ptr->magic != MODULE_MAGIC) {
            return module_load_result::invalid_magic;
        }

        // Check version compatibility
        version_info module_version{header_ptr->version_major, header_ptr->version_minor};
        if (!is_version_compatible(runtime_version, module_version)) {
            return module_load_result::unsupported_version;
        }

        // Check total size
        if (header_ptr->total_size > data_size) {
            return module_load_result::corrupt_data;
        }

        // Verify module hash
        const size_t hash_offset = offsetof(module_header, creation_timestamp);
        uint64_t computed_hash = FNV_OFFSET_BASIS;

        // Hash header (excluding magic, version, header_size, total_size, module_hash)
        const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(header_ptr);
        for (size_t i = hash_offset; i < sizeof(module_header); ++i) {
            computed_hash ^= header_bytes[i];
            computed_hash *= FNV_PRIME;
        }

        // Hash all data after header
        for (size_t i = sizeof(module_header); i < header_ptr->total_size; ++i) {
            computed_hash ^= data[i];
            computed_hash *= FNV_PRIME;
        }

        if (computed_hash != header_ptr->module_hash) {
            return module_load_result::hash_mismatch;
        }

        // Check capabilities
        const capability_flags required_caps = static_cast<capability_flags>(header_ptr->capabilities);
        const capability_flags missing_caps = static_cast<capability_flags>(
            static_cast<uint64_t>(required_caps) & ~static_cast<uint64_t>(available_caps)
        );
        
        if (static_cast<uint64_t>(missing_caps) != 0) {
            return module_load_result::insufficient_capabilities;
        }

        // Validate section headers
        const size_t sections_offset = sizeof(module_header);
        const size_t sections_size = sizeof(section_header) * header_ptr->section_count;
        
        if (sections_offset + sections_size > header_ptr->total_size) {
            return module_load_result::corrupt_data;
        }

        // Store header info
        header = *header_ptr;
        section_count = header_ptr->section_count;

        // Validate all sections
        for (size_t i = 0; i < section_count; ++i) {
            const section_header* sec_header = get_section_header(i);
            if (sec_header == nullptr) {
                return module_load_result::corrupt_data;
            }

            // Check section bounds
            if (sec_header->offset + sec_header->size > header_ptr->total_size) {
                return module_load_result::section_out_of_bounds;
            }

            // Verify section hash
            const uint8_t* section_data = data + sec_header->offset;
            const uint64_t section_hash = compute_hash(section_data, sec_header->size);
            if (section_hash != sec_header->hash) {
                return module_load_result::hash_mismatch;
            }
        }

        loaded = true;
        return module_load_result::success;
    }

    // Check if module is loaded
    [[nodiscard]] bool is_loaded() const noexcept {
        return loaded;
    }

    // Get module header
    [[nodiscard]] const module_header& get_header() const noexcept {
        return header;
    }

    // Get number of sections
    [[nodiscard]] size_t get_section_count() const noexcept {
        return section_count;
    }

    // Get section header by index
    [[nodiscard]] const section_header* get_section_header(size_t index) const noexcept {
        if (index >= section_count) {
            return nullptr;
        }

        const size_t offset = sizeof(module_header) + index * sizeof(section_header);
        if (offset + sizeof(section_header) > data_size) {
            return nullptr;
        }

        return reinterpret_cast<const section_header*>(data + offset);
    }

    // Find section by type
    [[nodiscard]] const section_header* find_section(section_type type) const noexcept {
        for (size_t i = 0; i < section_count; ++i) {
            const section_header* header_ptr = get_section_header(i);
            if (header_ptr && header_ptr->type == type) {
                return header_ptr;
            }
        }
        return nullptr;
    }

    // Find section by name
    [[nodiscard]] const section_header* find_section_by_name(const char* name) const noexcept {
        for (size_t i = 0; i < section_count; ++i) {
            const section_header* header_ptr = get_section_header(i);
            if (header_ptr && strings_equal(header_ptr->name, name, sizeof(header_ptr->name))) {
                return header_ptr;
            }
        }
        return nullptr;
    }

    // Get section data
    [[nodiscard]] const uint8_t* get_section_data(const section_header* section) const noexcept {
        if (!loaded || section == nullptr) {
            return nullptr;
        }

        if (section->offset + section->size > data_size) {
            return nullptr;
        }

        return data + section->offset;
    }

    // Get metadata section
    [[nodiscard]] const module_metadata* get_metadata() const noexcept {
        const section_header* meta_section = find_section(section_type::metadata);
        if (meta_section == nullptr || meta_section->size < sizeof(module_metadata)) {
            return nullptr;
        }

        return reinterpret_cast<const module_metadata*>(get_section_data(meta_section));
    }

    // Get code section
    [[nodiscard]] const uint8_t* get_code(size_t* out_size = nullptr) const noexcept {
        const section_header* code_section = find_section(section_type::code);
        if (code_section == nullptr) {
            return nullptr;
        }

        if (out_size != nullptr) {
            *out_size = code_section->size;
        }

        return get_section_data(code_section);
    }

    // Get assets section
    [[nodiscard]] const uint8_t* get_assets(size_t* out_size = nullptr) const noexcept {
        const section_header* assets_section = find_section(section_type::assets);
        if (assets_section == nullptr) {
            return nullptr;
        }

        if (out_size != nullptr) {
            *out_size = assets_section->size;
        }

        return get_section_data(assets_section);
    }

    // Find asset by name within assets section
    [[nodiscard]] const asset_entry* find_asset(const char* asset_name) const noexcept {
        size_t assets_size = 0;
        const uint8_t* assets_data = get_assets(&assets_size);
        
        if (assets_data == nullptr || assets_size < sizeof(asset_entry)) {
            return nullptr;
        }

        // Calculate number of asset entries
        const size_t num_entries = assets_size / sizeof(asset_entry);
        
        for (size_t i = 0; i < num_entries; ++i) {
            const asset_entry* entry = reinterpret_cast<const asset_entry*>(
                assets_data + i * sizeof(asset_entry)
            );
            
            if (strings_equal(entry->name, asset_name, sizeof(entry->name))) {
                return entry;
            }
            
            // Stop if we've gone past the entries into the data section
            if ((i + 1) * sizeof(asset_entry) > assets_size) {
                break;
            }
        }

        return nullptr;
    }

    // Get asset data
    [[nodiscard]] const uint8_t* get_asset_data(const asset_entry* asset) const noexcept {
        if (asset == nullptr) {
            return nullptr;
        }

        size_t assets_size = 0;
        const uint8_t* assets_section = get_assets(&assets_size);
        
        if (assets_section == nullptr) {
            return nullptr;
        }

        // Asset data follows the asset entries
        const size_t data_offset = asset->offset;
        if (data_offset + asset->size > assets_size) {
            return nullptr;
        }

        return assets_section + data_offset;
    }

  private:
    const uint8_t* data;
    size_t data_size;
    module_header header{};
    size_t section_count{0};
    bool loaded{false};

    // Helper to compare strings safely
    static bool strings_equal(const char* a, const char* b, size_t max_len) noexcept {
        for (size_t i = 0; i < max_len; ++i) {
            if (a[i] != b[i]) {
                return false;
            }
            if (a[i] == '\0') {
                return true;
            }
        }
        return true;
    }
};

} // namespace aegis::core::module
