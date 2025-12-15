#pragma once

#include "core/events/input_event.hpp"

#include <cstddef>
#include <cstdint>

// Runtime Platform Adapter Interface
// This is the ONLY place where OS APIs are allowed
// Maps OS-specific input to core/events types

namespace aegis::runtime::platform {

// Platform-specific event callback
// Called by platform layer when OS events arrive
// Returns true if event was handled
using event_callback_fn = bool (*)(const core::events::input_event& event,
                                   void* user_data) noexcept;

// Platform abstraction layer
// Implementations exist for each supported platform (Linux, Windows, macOS)
class platform_adapter {
  public:
    virtual ~platform_adapter() noexcept = default;

    // Initialize platform adapter
    // Returns true on success
    [[nodiscard]] virtual bool initialize() noexcept = 0;

    // Shutdown platform adapter
    virtual void shutdown() noexcept = 0;

    // Poll for OS events and convert to core events
    // Calls the registered callback for each event
    // Returns number of events processed
    virtual size_t poll_events() noexcept = 0;

    // Register event callback
    // Only one callback can be registered at a time
    virtual void set_event_callback(event_callback_fn callback, void* user_data) noexcept = 0;

    // Get current time in nanoseconds (monotonic clock)
    // Used for timestamping events deterministically
    [[nodiscard]] virtual uint64_t get_time_ns() const noexcept = 0;

    // Platform adapter is non-copyable
    platform_adapter(const platform_adapter&) = delete;
    platform_adapter& operator=(const platform_adapter&) = delete;

    // Platform adapter is non-movable
    platform_adapter(platform_adapter&&) = delete;
    platform_adapter& operator=(platform_adapter&&) = delete;

  protected:
    platform_adapter() noexcept = default;
};

// Event replay buffer for deterministic replay
// Stores events with timestamps for exact replay
// Note: Uses fixed-size array for simplicity. Capacity is clamped to default_capacity.
// In production, would use arena/frame allocator for dynamic sizing.
class event_replay_buffer {
  public:
    // Constructor with capacity clamping
    // If capacity > default_capacity, it will be clamped to default_capacity
    // If capacity < default_capacity, only 'capacity' events can be stored
    explicit event_replay_buffer(size_t capacity) noexcept
        : max_events(capacity > default_capacity ? default_capacity : capacity) {
    }

    // Add event to replay buffer
    [[nodiscard]] bool push(const core::events::input_event& event) noexcept {
        if (event_count >= max_events) {
            return false; // Buffer full
        }

        // In production, would use custom allocator
        // For now, using compile-time fixed capacity
        events[event_count] = event;
        event_count++;
        return true;
    }

    // Get event at index
    [[nodiscard]] const core::events::input_event* get(size_t index) const noexcept {
        if (index >= event_count) {
            return nullptr;
        }
        return &events[index];
    }

    // Replay all events through callback
    void replay_all(event_callback_fn callback, void* user_data) const noexcept {
        if (callback == nullptr) {
            return; // No callback registered
        }
        for (size_t i = 0; i < event_count; ++i) {
            callback(events[i], user_data);
        }
    }

    // Clear buffer
    void clear() noexcept {
        event_count = 0;
    }

    // Query functions
    [[nodiscard]] size_t count() const noexcept {
        return event_count;
    }

    [[nodiscard]] bool empty() const noexcept {
        return event_count == 0;
    }

    [[nodiscard]] bool full() const noexcept {
        return event_count >= max_events;
    }

    [[nodiscard]] size_t capacity() const noexcept {
        return max_events;
    }

  private:
    static constexpr size_t default_capacity = 1024;
    size_t max_events{default_capacity};
    core::events::input_event events[default_capacity]{};
    size_t event_count{0};
};

} // namespace aegis::runtime::platform
