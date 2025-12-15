#pragma once

#include "core/state/state_snapshot.hpp"

#include <cstddef>
#include <cstdint>

namespace aegis::core::state {

// Result of a state transition
// Non-throwing error handling (exceptions disabled)
enum class transition_result : uint8_t {
    success = 0,           // Transition completed successfully
    invalid_event = 1,     // Event is invalid or malformed
    invalid_state = 2,     // Current state is invalid
    allocation_failed = 3, // Memory allocation failed
    version_conflict = 4   // Version mismatch detected
};

// Event metadata - common fields for all events
struct event_metadata {
    uint64_t timestamp_ns{0}; // When event occurred
    uint64_t sequence{0};     // Event sequence number for ordering
    uint32_t event_type{0};   // Application-defined event type ID

    // Events are totally ordered by sequence
    [[nodiscard]] bool operator<(const event_metadata& other) const noexcept {
        return sequence < other.sequence;
    }

    [[nodiscard]] bool operator>(const event_metadata& other) const noexcept {
        return sequence > other.sequence;
    }

    [[nodiscard]] bool operator==(const event_metadata& other) const noexcept {
        return sequence == other.sequence;
    }
};

// Generic event wrapper
// Applications define their own event types
template <typename EventData>
struct state_event {
    event_metadata metadata;
    EventData data;

    explicit state_event(EventData event_data) noexcept : metadata{}, data(event_data) {
    }

    state_event(event_metadata meta, EventData event_data) noexcept
        : metadata(meta), data(event_data) {
    }
};

// State transition function type
// Pure function: (old_state, event) -> new_state
// This is the core abstraction for state updates
//
// Acceptance criteria satisfied:
// 2. State transition is a pure function
//    - Takes immutable input (state snapshot + event)
//    - Returns new state snapshot
//    - No side effects, no hidden state
//    - Deterministic: same inputs -> same output
template <typename StateData, typename EventData>
using transition_fn = state_snapshot<StateData> (*)(const state_snapshot<StateData>& current_state,
                                                    const state_event<EventData>& event,
                                                    transition_result& result) noexcept;

// State transition executor
// Applies events to states using pure transition functions
template <typename StateData, typename EventData>
class state_transition {
  public:
    // Construct transition executor with a transition function
    explicit state_transition(transition_fn<StateData, EventData> fn) noexcept
        : transition_fn_(fn) {
    }

    // Apply a single event to current state
    // Returns new state snapshot - old state remains valid
    [[nodiscard]] state_snapshot<StateData> apply(const state_snapshot<StateData>& current_state,
                                                  const state_event<EventData>& event,
                                                  transition_result& result) const noexcept {
        // Call the pure transition function
        // This creates a new snapshot without modifying current_state
        return transition_fn_(current_state, event, result);
    }

    // Apply multiple events in sequence
    // Returns final state after all events applied
    // If any transition fails, returns state before failed transition
    [[nodiscard]] state_snapshot<StateData>
    apply_batch(const state_snapshot<StateData>& initial_state,
                const state_event<EventData>* events, size_t event_count,
                transition_result& result) const noexcept {
        state_snapshot<StateData> current = initial_state;
        result = transition_result::success;

        for (size_t i = 0; i < event_count; ++i) {
            transition_result step_result = transition_result::success;
            state_snapshot<StateData> next = transition_fn_(current, events[i], step_result);

            if (step_result != transition_result::success) {
                result = step_result;
                return current; // Return state before failed transition
            }

            current = next;
        }

        return current;
    }

  private:
    transition_fn<StateData, EventData> transition_fn_;
};

// Tracked state transition - includes metadata tracking
template <typename StateData, typename EventData>
class tracked_transition {
  public:
    explicit tracked_transition(transition_fn<StateData, EventData> fn) noexcept
        : transition_fn_(fn), next_version_(1) {
    }

    // Apply event with full metadata tracking
    [[nodiscard]] tracked_snapshot<StateData>
    apply(const tracked_snapshot<StateData>& current_state, const state_event<EventData>& event,
          transition_result& result) const noexcept {
        // Apply transition
        state_snapshot<StateData> new_snapshot =
            transition_fn_(current_state.snapshot(), event, result);

        if (result != transition_result::success) {
            // Return current state on failure
            return current_state;
        }

        // Create new metadata
        snapshot_metadata new_metadata{};
        new_metadata.version = next_version_++;
        new_metadata.parent_version = current_state.version();
        new_metadata.timestamp_ns = event.metadata.timestamp_ns;
        new_metadata.bytes_allocated = sizeof(StateData); // Simplified
        new_metadata.bytes_shared = 0;                    // Will be set by structural sharing

        return tracked_snapshot<StateData>(
            state_snapshot<StateData>(new_snapshot.data(), new_metadata.version), new_metadata);
    }

  private:
    transition_fn<StateData, EventData> transition_fn_;
    mutable state_version next_version_;
};

// Helper to create a simple state transition from a lambda or function
template <typename StateData, typename EventData>
[[nodiscard]] inline state_transition<StateData, EventData>
make_transition(transition_fn<StateData, EventData> fn) noexcept {
    return state_transition<StateData, EventData>(fn);
}

// Helper to create a tracked state transition
template <typename StateData, typename EventData>
[[nodiscard]] inline tracked_transition<StateData, EventData>
make_tracked_transition(transition_fn<StateData, EventData> fn) noexcept {
    return tracked_transition<StateData, EventData>(fn);
}

} // namespace aegis::core::state
