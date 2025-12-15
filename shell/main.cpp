// Aegis Shell - Minimal viewer-runtime
// This is NOT a browser, it's SDL for Aegis
//
// Responsibilities:
// 1. Window creation
// 2. Input → event mapping
// 3. GPU surface
// 4. Module loading
//
// Zero logic in shell - all logic lives in core/

#include "core/events/input_event.hpp"
#include "core/module/module_loader.hpp"
#include "core/version.hpp"

#include "runtime/platform/null_platform.hpp"
#include "runtime/platform/platform_adapter.hpp"

#include <cstdio>
#include <cstring>

using namespace aegis::core;
using namespace aegis::core::events;
using namespace aegis::core::module;
using namespace aegis::runtime::platform;

// Shell configuration
struct shell_config {
    const char* module_path{nullptr};
    const char* record_path{nullptr};
    const char* replay_path{nullptr};
    bool show_help{false};
    bool show_version{false};
};

// Parse command line arguments
static shell_config parse_args(int argc, char** argv) {
    shell_config config{};

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            config.show_help = true;
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            config.show_version = true;
        } else if (strcmp(argv[i], "--record") == 0 && i + 1 < argc) {
            config.record_path = argv[++i];
        } else if (strcmp(argv[i], "--replay") == 0 && i + 1 < argc) {
            config.replay_path = argv[++i];
        } else if (config.module_path == nullptr && argv[i][0] != '-') {
            config.module_path = argv[i];
        }
    }

    return config;
}

// Print usage information
static void print_usage(const char* program_name) {
    printf("Aegis Shell - Minimal viewer-runtime for Aegis applications\n\n");
    printf("Usage: %s [options] <module.aegis>\n\n", program_name);
    printf("Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -v, --version           Show version information\n");
    printf("  --record <file>         Record events to file\n");
    printf("  --replay <file>         Replay events from file\n\n");
    printf("Examples:\n");
    printf("  %s app.aegis                    # Run module\n", program_name);
    printf("  %s app.aegis --record log.bin   # Run and record\n", program_name);
    printf("  %s app.aegis --replay log.bin   # Replay session\n", program_name);
}

// Print version information
static void print_version() {
    printf("Aegis Shell v%d.%d.%d\n", aegis::core::version_major, aegis::core::version_minor,
           aegis::core::version_patch);
    printf("Runtime: Aegis Core\n");
}

// Event handler - forwards to application logic in core/
static bool handle_event(const input_event& event, void* user_data) noexcept {
    // Shell has ZERO logic
    // Just forward event to application (would be in core/)
    // For now, just log it for demonstration
    (void)event;
    (void)user_data;
    return true;
}

int main(int argc, char** argv) {
    // Parse arguments
    shell_config config = parse_args(argc, argv);

    // Handle help and version
    if (config.show_help) {
        print_usage(argv[0]);
        return 0;
    }

    if (config.show_version) {
        print_version();
        return 0;
    }

    // Validate arguments
    if (config.module_path == nullptr) {
        fprintf(stderr, "Error: No module specified\n\n");
        print_usage(argv[0]);
        return 1;
    }

    printf("=== Aegis Shell ===\n");
    printf("Version: %d.%d.%d\n", aegis::core::version_major, aegis::core::version_minor,
           aegis::core::version_patch);
    printf("Module: %s\n", config.module_path);

    if (config.record_path != nullptr) {
        printf("Recording: %s\n", config.record_path);
    }

    if (config.replay_path != nullptr) {
        printf("Replaying: %s\n", config.replay_path);
    }

    printf("\n");

    // Step 1: Initialize platform adapter
    printf("1. Initializing platform...\n");
    null_platform_adapter platform;
    if (!platform.initialize()) {
        fprintf(stderr, "Error: Failed to initialize platform\n");
        return 1;
    }
    printf("   Platform initialized\n");

    // Step 2: Set event callback
    printf("2. Setting up event handling...\n");
    platform.set_event_callback(handle_event, nullptr);
    printf("   Event handler registered\n");

    // Step 3: Module loading
    printf("3. Loading module: %s\n", config.module_path);
    // Note: In a real implementation, would load .aegis file here
    // For now, just demonstrate the interface
    printf("   Module loading interface ready\n");
    printf("   (Actual .aegis loading requires file I/O in runtime layer)\n");

    // Step 4: Main loop (shell has zero logic)
    printf("4. Starting main loop...\n");
    printf("   Press Ctrl+C to exit\n");
    printf("\n");

    // Simulate a few frames
    for (int frame = 0; frame < 5; ++frame) {
        printf("Frame %d:\n", frame);

        // Poll events (zero logic, just delegation)
        size_t event_count = platform.poll_events();
        printf("  Polled %zu events\n", event_count);

        // In real implementation:
        // - Core would process events
        // - Core would update state
        // - Core would generate scene graph
        // - Runtime would render
        // Shell does NOTHING except bootstrap

        // Small delay to simulate frame time
        // (In real impl, would be VSync or fixed timestep from runtime)
    }

    printf("\n");

    // Step 5: Shutdown
    printf("5. Shutting down...\n");
    platform.shutdown();
    printf("   Platform shutdown complete\n");

    printf("\n=== Shell exited cleanly ===\n");
    return 0;
}
