#pragma once

#include "module_format.hpp"

#include <cstring>

namespace aegis::core::module {

// Module builder - constructs .aegis module files deterministically
// Same inputs → byte-identical outputs
class module_builder {
  public:
    // Create builder with output buffer
    // buffer must be large enough to hold the entire module
    explicit module_builder(uint8_t* buffer_ptr, size_t buffer_size_val) noexcept
        : buffer(buffer_ptr), buffer_size(buffer_size_val), assets_data(temp_assets_buffer) {
    }

    ~module_builder() noexcept = default;

    // Disable copy and move
    module_builder(const module_builder&) = delete;
    module_builder& operator=(const module_builder&) = delete;
    module_builder(module_builder&&) = delete;
    module_builder& operator=(module_builder&&) = delete;

    // Initialize module with metadata
    bool init(const char* module_name, const char* version_str, const char* author_name,
              const char* desc, capability_flags caps, uint64_t timestamp) noexcept {
        if (current_offset != 0) {
            return false; // Already initialized
        }

        // Reserve space for module header
        current_offset = sizeof(module_header);

        // Reserve space for section headers (we'll fill them in later)
        section_headers_offset = current_offset;
        current_offset += sizeof(section_header) * MAX_SECTIONS;

        // Write metadata section
        module_metadata meta{};
        copy_string(meta.name, module_name, sizeof(meta.name));
        copy_string(meta.version, version_str, sizeof(meta.version));
        copy_string(meta.author, author_name, sizeof(meta.author));
        copy_string(meta.description, desc, sizeof(meta.description));

        if (!add_section(section_type::metadata, "metadata",
                        reinterpret_cast<const uint8_t*>(&meta), sizeof(meta))) {
            return false;
        }

        stored_capabilities = static_cast<uint64_t>(caps);
        stored_timestamp = timestamp;
        return true;
    }

    // Add a section to the module
    bool add_section(section_type type, const char* section_name, const uint8_t* data,
                     size_t data_size) noexcept {
        if (section_count >= MAX_SECTIONS) {
            return false; // Too many sections
        }

        if (current_offset + data_size > buffer_size) {
            return false; // Not enough space
        }

        // Write section data
        for (size_t i = 0; i < data_size; ++i) {
            buffer[current_offset + i] = data[i];
        }

        // Create section header
        section_header& header = sections[section_count];
        header.type = type;
        header.offset = current_offset;
        header.size = data_size;
        header.hash = compute_hash(data, data_size);
        copy_string(header.name, section_name, sizeof(header.name));

        current_offset += data_size;
        section_count++;
        return true;
    }

    // Add code section
    bool add_code(const uint8_t* code_data, size_t code_size) noexcept {
        return add_section(section_type::code, "code", code_data, code_size);
    }

    // Add asset to module
    bool add_asset(const char* asset_name, uint32_t asset_type, const uint8_t* asset_data,
                   size_t asset_size) noexcept {
        if (assets_count >= MAX_ASSETS) {
            return false;
        }

        // Store asset entry
        asset_entry& entry = assets[assets_count];
        copy_string(entry.name, asset_name, sizeof(entry.name));
        entry.type = asset_type;
        entry.offset = assets_data_offset;
        entry.size = asset_size;
        entry.hash = compute_hash(asset_data, asset_size);

        // Store asset data
        if (assets_data_offset + asset_size > sizeof(temp_assets_buffer)) {
            return false; // Not enough space for assets
        }

        for (size_t i = 0; i < asset_size; ++i) {
            assets_data[assets_data_offset + i] = asset_data[i];
        }

        assets_data_offset += asset_size;
        assets_count++;
        return true;
    }

    // Finalize module and compute final hash
    // After this, the module is ready to be written to disk
    bool finalize() noexcept {
        // Write assets section if we have any
        if (assets_count > 0) {
            // Calculate total assets section size
            const size_t assets_header_size = sizeof(asset_entry) * assets_count;
            const size_t total_assets_size = assets_header_size + assets_data_offset;

            // Create temporary buffer for assets section
            if (current_offset + total_assets_size > buffer_size) {
                return false; // Not enough space
            }

            // Write asset entries
            uint8_t* assets_section = buffer + current_offset;
            for (size_t i = 0; i < assets_count; ++i) {
                const uint8_t* entry_bytes = reinterpret_cast<const uint8_t*>(&assets[i]);
                for (size_t j = 0; j < sizeof(asset_entry); ++j) {
                    assets_section[i * sizeof(asset_entry) + j] = entry_bytes[j];
                }
            }

            // Write asset data
            for (size_t i = 0; i < assets_data_offset; ++i) {
                assets_section[assets_header_size + i] = assets_data[i];
            }

            // Add assets section
            if (!add_section(section_type::assets, "assets", assets_section, total_assets_size)) {
                return false;
            }
        }

        // Write section headers
        uint8_t* section_headers_ptr = buffer + section_headers_offset;
        for (size_t i = 0; i < section_count; ++i) {
            const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&sections[i]);
            for (size_t j = 0; j < sizeof(section_header); ++j) {
                section_headers_ptr[i * sizeof(section_header) + j] = header_bytes[j];
            }
        }

        // Write module header
        module_header header{};
        header.magic = MODULE_MAGIC;
        header.version_major = MODULE_VERSION_MAJOR;
        header.version_minor = MODULE_VERSION_MINOR;
        header.header_size = sizeof(module_header);
        header.total_size = current_offset;
        header.creation_timestamp = stored_timestamp;
        header.capabilities = stored_capabilities;
        header.section_count = static_cast<uint32_t>(section_count);

        // Compute module hash (exclude the hash field itself)
        // Hash everything after the module_hash field
        const size_t hash_offset = offsetof(module_header, creation_timestamp);
        uint64_t hash = FNV_OFFSET_BASIS;

        // Hash the header (excluding magic, version, header_size, total_size, module_hash)
        const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&header);
        for (size_t i = hash_offset; i < sizeof(module_header); ++i) {
            hash ^= header_bytes[i];
            hash *= FNV_PRIME;
        }

        // Hash all sections and data
        for (size_t i = sizeof(module_header); i < current_offset; ++i) {
            hash ^= buffer[i];
            hash *= FNV_PRIME;
        }

        header.module_hash = hash;

        // Write final header to buffer
        const uint8_t* final_header_bytes = reinterpret_cast<const uint8_t*>(&header);
        for (size_t i = 0; i < sizeof(module_header); ++i) {
            buffer[i] = final_header_bytes[i];
        }

        return true;
    }

    // Get the final module size
    [[nodiscard]] size_t get_module_size() const noexcept {
        return current_offset;
    }

    // Get the module buffer
    [[nodiscard]] const uint8_t* get_buffer() const noexcept {
        return buffer;
    }

  private:
    static constexpr size_t MAX_SECTIONS = 16;
    static constexpr size_t MAX_ASSETS = 64;
    static constexpr size_t MAX_ASSETS_DATA_SIZE = 1024 * 1024; // 1 MB

    uint8_t* buffer;
    size_t buffer_size;
    size_t current_offset{0};
    size_t section_headers_offset{0};
    size_t section_count{0};
    uint64_t stored_capabilities{0};
    uint64_t stored_timestamp{0};

    section_header sections[MAX_SECTIONS]{};
    asset_entry assets[MAX_ASSETS]{};
    uint8_t* assets_data; // Will point to temp buffer in caller's space
    size_t assets_count{0};
    size_t assets_data_offset{0};
    uint8_t temp_assets_buffer[16384]{}; // 16KB for assets during build

    // Helper to safely copy strings
    static void copy_string(char* dest, const char* src, size_t dest_size) noexcept {
        if (src == nullptr) {
            dest[0] = '\0';
            return;
        }

        size_t i = 0;
        while (i < dest_size - 1 && src[i] != '\0') {
            dest[i] = src[i];
            ++i;
        }
        dest[i] = '\0';

        // Zero remaining bytes for determinism
        for (size_t j = i + 1; j < dest_size; ++j) {
            dest[j] = '\0';
        }
    }
};

} // namespace aegis::core::module
