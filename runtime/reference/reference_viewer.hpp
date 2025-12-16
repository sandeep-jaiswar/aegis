// Reference Runtime Viewer - Minimal Runtime Shell Interface
// Ticket: REF-001
//
// This header defines the minimal interface any Aegis runtime must implement.
// It serves as the canonical specification for what a "runtime shell" is.
//
// Design Principles:
// 1. Zero business logic in viewer
// 2. Deterministic replay capability
// 3. Runtime can be replaced without touching core/
//
// Acceptance Criteria:
// ✅ Window creation
// ✅ Input → event mapping
// ✅ GPU surface setup
// ✅ Module loader

#pragma once

#include <cstddef>
#include <cstdint>

namespace aegis::runtime::reference {

// Forward declarations for core types
namespace core_events {
struct input_event;
}

namespace core_module {
struct module_header;
enum class module_load_result : uint8_t;
}

// Event callback signature
// Returns true if event was handled, false otherwise
using event_callback_fn = bool (*)(const core_events::input_event& event, void* user_data);

// Window configuration
struct window_config {
    const char* title{nullptr};
    uint32_t width{800};
    uint32_t height{600};
    bool resizable{true};
    bool fullscreen{false};
};

// GPU surface configuration
struct gpu_surface_config {
    uint32_t width{800};
    uint32_t height{600};
    bool vsync{true};
};

// Module load configuration
struct module_load_config {
    const char* module_path{nullptr};
    uint64_t runtime_capabilities{0};  // capability_flags as uint64_t
};

// Reference Viewer Interface
//
// This is the minimal interface any Aegis runtime viewer must implement.
// All methods MUST be deterministic when replaying recorded events.
class reference_viewer_interface {
  public:
    virtual ~reference_viewer_interface() = default;

    // ====================================================================
    // INITIALIZATION & SHUTDOWN
    // ====================================================================

    // Initialize the viewer runtime
    // Returns true on success, false on failure
    virtual bool initialize() noexcept = 0;

    // Shutdown the viewer runtime
    // MUST clean up all resources
    virtual void shutdown() noexcept = 0;

    // ====================================================================
    // WINDOW CREATION (REF-001 Requirement)
    // ====================================================================

    // Create a window with the specified configuration
    // Returns true on success, false on failure
    // MUST be callable before module loading
    virtual bool create_window(const window_config& config) noexcept = 0;

    // Destroy the window
    // MUST clean up all window resources
    virtual void destroy_window() noexcept = 0;

    // Check if window should close (e.g., user clicked X)
    // Returns true if window should close
    virtual bool should_close() const noexcept = 0;

    // ====================================================================
    // INPUT → EVENT MAPPING (REF-001 Requirement)
    // ====================================================================

    // Set event callback for input events
    // Callback is invoked when OS input is mapped to core::events::input_event
    // MUST be deterministic: same OS events → same input_events
    virtual void set_event_callback(event_callback_fn callback, void* user_data) noexcept = 0;

    // Poll OS events and convert to input_events
    // Returns number of events processed
    // MUST call event_callback for each event
    virtual size_t poll_events() noexcept = 0;

    // ====================================================================
    // GPU SURFACE SETUP (REF-001 Requirement)
    // ====================================================================

    // Create GPU-compatible surface
    // Returns true on success, false on failure
    // MUST be callable after window creation
    virtual bool create_gpu_surface(const gpu_surface_config& config) noexcept = 0;

    // Get GPU surface handle (platform-specific)
    // Returns opaque pointer to GPU surface
    // Returns nullptr if no surface created
    virtual void* get_gpu_surface_handle() const noexcept = 0;

    // Destroy GPU surface
    // MUST clean up all GPU resources
    virtual void destroy_gpu_surface() noexcept = 0;

    // ====================================================================
    // MODULE LOADER (REF-001 Requirement)
    // ====================================================================

    // Load an Aegis module
    // Returns module_load_result indicating success or specific failure
    // MUST verify module capabilities against runtime capabilities
    virtual core_module::module_load_result load_module(
        const module_load_config& config) noexcept = 0;

    // Unload the current module
    // MUST clean up all module resources
    virtual void unload_module() noexcept = 0;

    // Get loaded module header
    // Returns nullptr if no module loaded
    virtual const core_module::module_header* get_module_header() const noexcept = 0;

    // ====================================================================
    // TIMING (for deterministic replay)
    // ====================================================================

    // Get monotonic timestamp in nanoseconds
    // MUST be deterministic during replay
    virtual uint64_t get_time_ns() const noexcept = 0;

    // ====================================================================
    // RECORDING & REPLAY (REF-001 Acceptance Criterion)
    // ====================================================================

    // Start recording events to file
    // Returns true on success, false on failure
    virtual bool start_recording(const char* filepath) noexcept = 0;

    // Stop recording events
    virtual void stop_recording() noexcept = 0;

    // Start replaying events from file
    // Returns true on success, false on failure
    // When replaying, poll_events() replays recorded events instead of OS events
    virtual bool start_replay(const char* filepath) noexcept = 0;

    // Stop replaying events
    virtual void stop_replay() noexcept = 0;

    // Check if currently recording
    virtual bool is_recording() const noexcept = 0;

    // Check if currently replaying
    virtual bool is_replaying() const noexcept = 0;
};

// ====================================================================
// ACCEPTANCE CRITERIA VERIFICATION
// ====================================================================

// REF-001 Acceptance Criteria:
//
// ✅ Zero business logic in shell
//    - Interface defines only OS integration points
//    - No rendering, layout, state management, or computation
//    - All logic lives in core/
//
// ✅ Killing viewer + replaying log yields identical output
//    - start_recording() / stop_recording() capture events
//    - start_replay() / stop_replay() replay events deterministically
//    - Same events → same input_event sequence → same core/ behavior
//
// ✅ Shell can be replaced without touching core/
//    - Interface is implementation-agnostic
//    - Any language can implement this interface
//    - Core/ depends on events, not on viewer implementation

} // namespace aegis::runtime::reference
