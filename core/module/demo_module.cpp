#include "module_builder.hpp"
#include "module_loader.hpp"

#include <cstdio>
#include <cstring>

namespace aegis::core::module {

// Demo: Create and load a .aegis module
void demo_module_format() {
    printf("=== Aegis Module Format Demo ===\n\n");

    // Step 1: Create a module
    printf("Step 1: Building module...\n");
    
    constexpr size_t BUFFER_SIZE = 1024 * 1024; // 1 MB buffer
    uint8_t module_buffer[BUFFER_SIZE]{};
    
    module_builder builder(module_buffer, BUFFER_SIZE);

    // Initialize module with metadata
    const capability_flags caps = capability_flags::gpu_rendering | capability_flags::input_events;
    const uint64_t timestamp = 1702656000; // Example timestamp
    
    bool success = builder.init(
        "demo_app",                         // name
        "1.0.0",                           // version
        "Aegis Team",                      // author
        "Demo application showcasing the Aegis module format", // description
        caps,                              // capabilities
        timestamp                          // creation timestamp
    );

    if (!success) {
        printf("ERROR: Failed to initialize module builder\n");
        return;
    }

    // Add some code section (dummy data for demo)
    uint8_t code_data[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f}; // "Hello"
    if (!builder.add_code(code_data, sizeof(code_data))) {
        printf("ERROR: Failed to add code section\n");
        return;
    }
    printf("  - Added code section (%zu bytes)\n", sizeof(code_data));

    // Add some assets
    uint8_t texture_data[] = {0xFF, 0x00, 0xFF, 0x00}; // Dummy texture
    if (!builder.add_asset("main_texture", 1, texture_data, sizeof(texture_data))) {
        printf("ERROR: Failed to add texture asset\n");
        return;
    }
    printf("  - Added asset: main_texture (%zu bytes)\n", sizeof(texture_data));

    uint8_t font_data[] = {0x46, 0x4f, 0x4e, 0x54}; // "FONT"
    if (!builder.add_asset("default_font", 2, font_data, sizeof(font_data))) {
        printf("ERROR: Failed to add font asset\n");
        return;
    }
    printf("  - Added asset: default_font (%zu bytes)\n", sizeof(font_data));

    // Finalize the module
    if (!builder.finalize()) {
        printf("ERROR: Failed to finalize module\n");
        return;
    }

    const size_t module_size = builder.get_module_size();
    printf("  - Module built successfully (%zu bytes)\n", module_size);
    printf("  - Module hash: 0x%016lx\n\n", 
           static_cast<unsigned long>(reinterpret_cast<const module_header*>(builder.get_buffer())->module_hash));

    // Step 2: Load and validate the module
    printf("Step 2: Loading module...\n");

    module_loader loader(builder.get_buffer(), module_size);

    // Current runtime version
    version_info runtime_version{MODULE_VERSION_MAJOR, MODULE_VERSION_MINOR};

    // Available capabilities in runtime
    capability_flags available_caps = capability_flags::all;

    module_load_result load_result = loader.load(runtime_version, available_caps);

    if (load_result != module_load_result::success) {
        printf("ERROR: Failed to load module (error code: %d)\n", static_cast<int>(load_result));
        return;
    }

    printf("  - Module loaded successfully!\n");

    // Step 3: Examine module contents
    printf("\nStep 3: Examining module contents...\n");

    const module_header& header = loader.get_header();
    printf("  - Magic: 0x%08X\n", header.magic);
    printf("  - Version: %d.%d\n", header.version_major, header.version_minor);
    printf("  - Total size: %lu bytes\n", static_cast<unsigned long>(header.total_size));
    printf("  - Creation timestamp: %lu\n", static_cast<unsigned long>(header.creation_timestamp));
    printf("  - Capabilities: 0x%016lx\n", static_cast<unsigned long>(header.capabilities));
    printf("  - Sections: %u\n", header.section_count);

    // Get metadata
    const module_metadata* metadata = loader.get_metadata();
    if (metadata != nullptr) {
        printf("\nMetadata:\n");
        printf("  - Name: %s\n", metadata->name);
        printf("  - Version: %s\n", metadata->version);
        printf("  - Author: %s\n", metadata->author);
        printf("  - Description: %s\n", metadata->description);
    }

    // Get code section
    size_t code_size = 0;
    const uint8_t* code = loader.get_code(&code_size);
    if (code != nullptr) {
        printf("\nCode section: %zu bytes\n", code_size);
        printf("  Data: ");
        for (size_t i = 0; i < code_size && i < 16; ++i) {
            printf("%02X ", code[i]);
        }
        printf("\n");
    }

    // Get assets
    printf("\nAssets:\n");
    
    const asset_entry* texture = loader.find_asset("main_texture");
    if (texture != nullptr) {
        printf("  - Found asset: %s\n", texture->name);
        printf("    Type: %u, Size: %lu bytes, Hash: 0x%016lx\n",
               texture->type, static_cast<unsigned long>(texture->size), 
               static_cast<unsigned long>(texture->hash));
        
        const uint8_t* texture_data_loaded = loader.get_asset_data(texture);
        if (texture_data_loaded != nullptr) {
            printf("    Data: ");
            for (size_t i = 0; i < texture->size && i < 8; ++i) {
                printf("%02X ", texture_data_loaded[i]);
            }
            printf("\n");
        }
    }

    const asset_entry* font = loader.find_asset("default_font");
    if (font != nullptr) {
        printf("  - Found asset: %s\n", font->name);
        printf("    Type: %u, Size: %lu bytes, Hash: 0x%016lx\n",
               font->type, static_cast<unsigned long>(font->size), 
               static_cast<unsigned long>(font->hash));
    }

    // Step 4: Test version compatibility
    printf("\nStep 4: Testing version compatibility...\n");

    version_info compatible_runtime{1, 0};
    printf("  - Runtime 1.0 with Module 1.0: %s\n",
           is_version_compatible(compatible_runtime, {1, 0}) ? "COMPATIBLE" : "INCOMPATIBLE");

    printf("  - Runtime 1.1 with Module 1.0: %s\n",
           is_version_compatible({1, 1}, {1, 0}) ? "COMPATIBLE" : "INCOMPATIBLE");

    printf("  - Runtime 1.0 with Module 1.1: %s\n",
           is_version_compatible({1, 0}, {1, 1}) ? "COMPATIBLE" : "INCOMPATIBLE");

    printf("  - Runtime 2.0 with Module 1.0: %s\n",
           is_version_compatible({2, 0}, {1, 0}) ? "COMPATIBLE" : "INCOMPATIBLE");

    // Step 5: Test determinism
    printf("\nStep 5: Testing determinism...\n");
    
    uint8_t module_buffer2[BUFFER_SIZE]{};
    module_builder builder2(module_buffer2, BUFFER_SIZE);
    
    // Build identical module
    builder2.init("demo_app", "1.0.0", "Aegis Team",
                  "Demo application showcasing the Aegis module format",
                  caps, timestamp);
    builder2.add_code(code_data, sizeof(code_data));
    builder2.add_asset("main_texture", 1, texture_data, sizeof(texture_data));
    builder2.add_asset("default_font", 2, font_data, sizeof(font_data));
    builder2.finalize();

    // Compare the two modules
    bool identical = true;
    for (size_t i = 0; i < module_size; ++i) {
        if (module_buffer[i] != module_buffer2[i]) {
            identical = false;
            break;
        }
    }

    printf("  - Two identical builds produce byte-identical output: %s\n",
           identical ? "YES ✓" : "NO ✗");

    printf("\n=== Demo Complete ===\n");
}

} // namespace aegis::core::module

int main() {
    aegis::core::module::demo_module_format();
    return 0;
}
