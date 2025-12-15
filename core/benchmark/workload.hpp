#pragma once

#include <cstddef>
#include <cstdint>

namespace aegis::core::benchmark {

// Workload event types for recording deterministic operations
enum class workload_event_type : uint8_t {
    allocate = 0,
    deallocate = 1,
    operation = 2,
    timestamp = 3,
    custom = 4
};

// Workload event - represents a single recorded operation
struct workload_event {
    workload_event_type type{workload_event_type::operation};
    uint64_t timestamp_ns{0};
    uint64_t param1{0}; // Size for allocations, operation ID, etc.
    uint64_t param2{0}; // Alignment for allocations, etc.
    uint64_t param3{0}; // Additional parameter
};

// Workload - sequence of events that can be replayed deterministically
struct workload {
    const char* name{nullptr};
    workload_event* events{nullptr};
    size_t event_count{0};
    size_t event_capacity{0};
    uint64_t hash{0}; // Hash of all events for verification
};

// Workload recorder - captures operations for deterministic replay
class workload_recorder {
  public:
    // Create recorder with event buffer
    explicit workload_recorder(workload_event* event_buffer_ptr,
                              size_t capacity_val) noexcept
        : event_buffer(event_buffer_ptr), capacity(capacity_val) {
    }

    ~workload_recorder() noexcept = default;

    // Disable copy and move
    workload_recorder(const workload_recorder&) = delete;
    workload_recorder& operator=(const workload_recorder&) = delete;
    workload_recorder(workload_recorder&&) = delete;
    workload_recorder& operator=(workload_recorder&&) = delete;

    // Start recording a new workload
    void start_recording(const char* workload_name) noexcept {
        event_count = 0;
        name = workload_name;
        current_hash = 0;
    }

    // Record an event
    bool record_event(workload_event_type type,
                     uint64_t timestamp_ns,
                     uint64_t param1 = 0,
                     uint64_t param2 = 0,
                     uint64_t param3 = 0) noexcept {
        if (event_count >= capacity) {
            return false;
        }

        workload_event& evt = event_buffer[event_count++];
        evt.type = type;
        evt.timestamp_ns = timestamp_ns;
        evt.param1 = param1;
        evt.param2 = param2;
        evt.param3 = param3;

        // Update hash (simple FNV-1a hash)
        current_hash ^= static_cast<uint64_t>(type);
        current_hash *= 0x100000001b3ULL;
        current_hash ^= timestamp_ns;
        current_hash *= 0x100000001b3ULL;
        current_hash ^= param1;
        current_hash *= 0x100000001b3ULL;

        return true;
    }

    // Stop recording and return workload
    [[nodiscard]] workload finish_recording() noexcept {
        workload w{};
        w.name = name;
        w.events = event_buffer;
        w.event_count = event_count;
        w.event_capacity = capacity;
        w.hash = current_hash;
        return w;
    }

    // Get current event count
    [[nodiscard]] size_t get_event_count() const noexcept {
        return event_count;
    }

  private:
    workload_event* event_buffer;
    size_t capacity;
    size_t event_count{0};
    const char* name{nullptr};
    uint64_t current_hash{0x811c9dc5}; // FNV-1a offset basis
};

// Workload player - replays recorded workloads deterministically
class workload_player {
  public:
    explicit workload_player(const workload& w) noexcept : work(w), current_index(0) {
    }

    ~workload_player() noexcept = default;

    // Disable copy and move
    workload_player(const workload_player&) = delete;
    workload_player& operator=(const workload_player&) = delete;
    workload_player(workload_player&&) = delete;
    workload_player& operator=(workload_player&&) = delete;

    // Reset to beginning of workload
    void reset() noexcept {
        current_index = 0;
    }

    // Get next event
    [[nodiscard]] const workload_event* next_event() noexcept {
        if (current_index >= work.event_count) {
            return nullptr;
        }
        return &work.events[current_index++];
    }

    // Check if more events remain
    [[nodiscard]] bool has_more_events() const noexcept {
        return current_index < work.event_count;
    }

    // Get current position
    [[nodiscard]] size_t get_position() const noexcept {
        return current_index;
    }

    // Get workload hash for verification
    [[nodiscard]] uint64_t get_hash() const noexcept {
        return work.hash;
    }

  private:
    const workload& work;
    size_t current_index;
};

} // namespace aegis::core::benchmark
