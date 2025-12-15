#pragma once

#include "runtime/platform/platform_adapter.hpp"

namespace aegis::runtime::platform {

// Null platform adapter for testing and headless mode
// Does not interact with any OS APIs
// Useful for deterministic testing and replay
class null_platform_adapter : public platform_adapter {
  public:
    null_platform_adapter() noexcept = default;
    ~null_platform_adapter() noexcept override = default;

    [[nodiscard]] bool initialize() noexcept override {
        initialized = true;
        start_time_ns = 0;
        return true;
    }

    void shutdown() noexcept override {
        initialized = false;
        callback = nullptr;
        user_data = nullptr;
    }

    size_t poll_events() noexcept override {
        // Null platform has no events
        return 0;
    }

    void set_event_callback(event_callback_fn cb, void* data) noexcept override {
        callback = cb;
        user_data = data;
    }

    [[nodiscard]] uint64_t get_time_ns() const noexcept override {
        // Return deterministic time for testing
        return current_time_ns;
    }

    // Testing helpers - inject events manually
    void inject_event(const core::events::input_event& event) noexcept {
        if (callback != nullptr) {
            callback(event, user_data);
        }
    }

    void advance_time(uint64_t delta_ns) noexcept {
        current_time_ns += delta_ns;
    }

    void set_time(uint64_t time_ns) noexcept {
        current_time_ns = time_ns;
    }

  private:
    bool initialized{false};
    event_callback_fn callback{nullptr};
    void* user_data{nullptr};
    uint64_t start_time_ns{0};
    uint64_t current_time_ns{0};
};

} // namespace aegis::runtime::platform
