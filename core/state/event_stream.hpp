#pragma once

#include "core/state/state_transition.hpp"

#include <cstddef>
#include <cstdint>

namespace aegis::core::state {

// Forward declarations
template <typename EventData>
class event_stream;

// Event stream configuration
struct event_stream_config {
    size_t max_events{1024};      // Maximum number of events to store
    bool auto_sequence{true};     // Automatically assign sequence numbers
    uint64_t initial_sequence{0}; // Starting sequence number
};

// Event stream result - for non-throwing error handling
enum class event_stream_result : uint8_t {
    success = 0,
    stream_full = 1,       // Maximum events reached
    invalid_event = 2,     // Event validation failed
    sequence_error = 3,    // Sequence number conflict
    empty_stream = 4,      // No events in stream
    allocation_failed = 5, // Memory allocation failed
    replay_failed = 6      // Replay operation failed
};

// Event stream statistics
struct event_stream_stats {
    size_t total_events{0};    // Total events ever added
    size_t current_events{0};  // Events currently in stream
    size_t events_replayed{0}; // Number of replay operations
    uint64_t min_sequence{0};  // Minimum sequence number in stream
    uint64_t max_sequence{0};  // Maximum sequence number in stream
    size_t bytes_allocated{0}; // Memory used by events
};

// Deterministic event stream
// Stores events in strictly ordered sequence for deterministic replay
//
// Design principles:
// 1. Events are totally ordered by sequence number
// 2. No callbacks - explicit iteration/replay only
// 3. Deterministic: replaying same stream produces identical state
// 4. Bounded memory: fixed capacity, no unbounded growth
//
// Thread safety: NOT thread-safe - caller must synchronize
template <typename EventData>
class event_stream {
  public:
    // Construct event stream with configuration
    explicit event_stream(const event_stream_config& config = {}) noexcept
        : config_(config), events_storage_{}, events_count_(0),
          next_sequence_(config.initial_sequence) {
        // Note: In production, would use custom allocator
        // For now, using compile-time fixed capacity
        // events_storage_ is raw memory and doesn't need initialization
        static_assert(sizeof(EventData) >= 1, "EventData must be a complete type");
    }

    ~event_stream() noexcept = default;

    // Disable copy - event stream owns its events
    event_stream(const event_stream&) = delete;
    event_stream& operator=(const event_stream&) = delete;

    // Allow move
    event_stream(event_stream&&) noexcept = default;
    event_stream& operator=(event_stream&&) noexcept = default;

    // Add event to stream with automatic sequencing
    // Returns the assigned sequence number on success
    [[nodiscard]] event_stream_result push(const EventData& data, uint64_t timestamp_ns,
                                           uint64_t* out_sequence = nullptr) noexcept {
        if (events_count_ >= config_.max_events) {
            return event_stream_result::stream_full;
        }

        // Create event with metadata
        event_metadata meta{};
        meta.timestamp_ns = timestamp_ns;
        meta.sequence = next_sequence_;
        meta.event_type = 0; // Application can set this

        // Store event
        events_ptr()[events_count_] = state_event<EventData>(meta, data);
        events_count_++;

        // Update statistics
        stats_.total_events++;
        stats_.current_events = events_count_;
        stats_.max_sequence = next_sequence_;
        if (events_count_ == 1) {
            stats_.min_sequence = next_sequence_;
        }
        stats_.bytes_allocated = events_count_ * sizeof(state_event<EventData>);

        if (out_sequence != nullptr) {
            *out_sequence = next_sequence_;
        }

        next_sequence_++;
        return event_stream_result::success;
    }

    // Add event with explicit metadata
    [[nodiscard]] event_stream_result push(const state_event<EventData>& event) noexcept {
        if (events_count_ >= config_.max_events) {
            return event_stream_result::stream_full;
        }

        // Validate sequence ordering if auto-sequencing is disabled
        if (!config_.auto_sequence && events_count_ > 0) {
            const uint64_t last_seq = events_ptr()[events_count_ - 1].metadata.sequence;
            if (event.metadata.sequence <= last_seq) {
                return event_stream_result::sequence_error;
            }
        }

        // Store event
        events_ptr()[events_count_] = event;
        events_count_++;

        // Update statistics
        stats_.total_events++;
        stats_.current_events = events_count_;
        stats_.max_sequence = event.metadata.sequence;
        if (events_count_ == 1) {
            stats_.min_sequence = event.metadata.sequence;
        }
        stats_.bytes_allocated = events_count_ * sizeof(state_event<EventData>);

        // Update next_sequence to maintain ordering
        if (event.metadata.sequence >= next_sequence_) {
            next_sequence_ = event.metadata.sequence + 1;
        }

        return event_stream_result::success;
    }

    // Replay all events through a transition function
    // Applies events in sequence order to produce final state
    // This is the core determinism guarantee:
    //   Same initial state + same event stream -> identical final state
    template <typename StateData>
    [[nodiscard]] state_snapshot<StateData>
    replay(const state_snapshot<StateData>& initial_state,
           transition_fn<StateData, EventData> transition_fn,
           event_stream_result& result) const noexcept {
        if (events_count_ == 0) {
            result = event_stream_result::empty_stream;
            return initial_state;
        }

        state_snapshot<StateData> current = initial_state;
        result = event_stream_result::success;

        // Apply events in order
        for (size_t i = 0; i < events_count_; ++i) {
            transition_result trans_result = transition_result::success;
            state_snapshot<StateData> next = transition_fn(current, events_ptr()[i], trans_result);

            if (trans_result != transition_result::success) {
                result = event_stream_result::replay_failed;
                return current; // Return state before failure
            }

            current = next;
        }

        // Update replay statistics
        stats_.events_replayed++;

        return current;
    }

    // Replay events through a state_transition object
    template <typename StateData>
    [[nodiscard]] state_snapshot<StateData>
    replay(const state_snapshot<StateData>& initial_state,
           const state_transition<StateData, EventData>& transition,
           event_stream_result& result) const noexcept {
        if (events_count_ == 0) {
            result = event_stream_result::empty_stream;
            return initial_state;
        }

        // Use batch apply for efficiency
        transition_result trans_result = transition_result::success;
        state_snapshot<StateData> final_state =
            transition.apply_batch(initial_state, events_ptr(), events_count_, trans_result);

        if (trans_result != transition_result::success) {
            result = event_stream_result::replay_failed;
        } else {
            result = event_stream_result::success;
            stats_.events_replayed++;
        }

        return final_state;
    }

    // Get event by index (0-based)
    // Returns nullptr if index is out of bounds
    [[nodiscard]] const state_event<EventData>* get(size_t index) const noexcept {
        if (index >= events_count_) {
            return nullptr;
        }
        return &events_ptr()[index];
    }

    // Iterate over events in order
    // Callback signature: bool callback(const state_event<EventData>&)
    // Return false from callback to stop iteration
    template <typename Callback>
    void for_each(Callback callback) const noexcept {
        for (size_t i = 0; i < events_count_; ++i) {
            if (!callback(events_ptr()[i])) {
                break;
            }
        }
    }

    // Clear all events (reset stream)
    void clear() noexcept {
        events_count_ = 0;
        stats_.current_events = 0;
        stats_.bytes_allocated = 0;
        // Note: total_events and events_replayed are cumulative
    }

    // Reset stream completely (including statistics and sequence)
    void reset() noexcept {
        events_count_ = 0;
        next_sequence_ = config_.initial_sequence;
        stats_ = event_stream_stats{};
    }

    // Query functions
    [[nodiscard]] size_t count() const noexcept {
        return events_count_;
    }

    [[nodiscard]] bool empty() const noexcept {
        return events_count_ == 0;
    }

    [[nodiscard]] bool full() const noexcept {
        return events_count_ >= config_.max_events;
    }

    [[nodiscard]] size_t capacity() const noexcept {
        return config_.max_events;
    }

    [[nodiscard]] const event_stream_stats& stats() const noexcept {
        return stats_;
    }

    [[nodiscard]] const event_stream_config& config() const noexcept {
        return config_;
    }

    // Get underlying event array for batch processing
    [[nodiscard]] const state_event<EventData>* events() const noexcept {
        return events_ptr();
    }

  private:
    event_stream_config config_;
    // Use aligned storage to avoid default construction requirement
    alignas(state_event<EventData>) unsigned char events_storage_[1024 *
                                                                  sizeof(state_event<EventData>)];
    size_t events_count_{0};
    uint64_t next_sequence_{0};
    mutable event_stream_stats stats_{}; // Mutable for replay count

    // Helper to get event array pointer
    [[nodiscard]] state_event<EventData>* events_ptr() noexcept {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<state_event<EventData>*>(events_storage_);
    }

    [[nodiscard]] const state_event<EventData>* events_ptr() const noexcept {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<const state_event<EventData>*>(events_storage_);
    }
};

// Helper to create an event stream with default configuration
template <typename EventData>
[[nodiscard]] inline event_stream<EventData> make_event_stream() noexcept {
    return event_stream<EventData>();
}

// Helper to create an event stream with custom configuration
template <typename EventData>
[[nodiscard]] inline event_stream<EventData>
make_event_stream(const event_stream_config& config) noexcept {
    return event_stream<EventData>(config);
}

} // namespace aegis::core::state
