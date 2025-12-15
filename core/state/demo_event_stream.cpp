#include "core/state/event_stream.hpp"
#include "core/state/state_snapshot.hpp"
#include "core/state/state_transition.hpp"

#include <cstdint>
#include <cstdio>

using namespace aegis::core::state;

// Example application state
struct counter_state {
    int32_t value{0};
    int32_t total_increments{0};
    int32_t total_decrements{0};

    bool operator==(const counter_state& other) const noexcept {
        return value == other.value && total_increments == other.total_increments &&
               total_decrements == other.total_decrements;
    }
};

// Example events
struct increment_event {
    int32_t amount{1};
};

struct decrement_event {
    int32_t amount{1};
};

struct reset_event {
    int32_t new_value{0};
};

// Event type discriminator
enum class event_type : uint32_t { increment = 1, decrement = 2, reset = 3 };

// Union event type for demonstration
struct counter_event {
    event_type type;
    union {
        increment_event increment;
        decrement_event decrement;
        reset_event reset;
    } data;

    static counter_event make_increment(int32_t amount) noexcept {
        counter_event e{};
        e.type = event_type::increment;
        e.data.increment.amount = amount;
        return e;
    }

    static counter_event make_decrement(int32_t amount) noexcept {
        counter_event e{};
        e.type = event_type::decrement;
        e.data.decrement.amount = amount;
        return e;
    }

    static counter_event make_reset(int32_t value) noexcept {
        counter_event e{};
        e.type = event_type::reset;
        e.data.reset.new_value = value;
        return e;
    }
};

// Pure transition function
state_snapshot<counter_state> handle_counter_event(const state_snapshot<counter_state>& current,
                                                   const state_event<counter_event>& event,
                                                   transition_result& result) noexcept {
    const counter_state& old_state = current.data();
    counter_state new_state = old_state;

    // Apply event based on type
    switch (event.data.type) {
        case event_type::increment:
            new_state.value += event.data.data.increment.amount;
            new_state.total_increments++;
            break;

        case event_type::decrement:
            new_state.value -= event.data.data.decrement.amount;
            new_state.total_decrements++;
            break;

        case event_type::reset:
            new_state.value = event.data.data.reset.new_value;
            break;
    }

    result = transition_result::success;
    return state_snapshot<counter_state>(new_state, current.version() + 1);
}

void print_state(const char* label, const state_snapshot<counter_state>& snapshot) noexcept {
    const counter_state& state = snapshot.data();
    printf("%s: value=%d, increments=%d, decrements=%d, version=%lu\n", label, state.value,
           state.total_increments, state.total_decrements, snapshot.version());
}

void print_separator() noexcept {
    printf("\n----------------------------------------\n\n");
}

int main() {
    printf("Aegis Deterministic Event Stream Demo\n");
    printf("======================================\n\n");

    // Create initial state
    state_snapshot<counter_state> initial_state(counter_state{0, 0, 0}, 0);
    print_state("Initial state", initial_state);
    print_separator();

    // Create event stream
    printf("Creating event stream with capacity 1024...\n");
    event_stream<counter_event> stream = make_event_stream<counter_event>();
    printf("Stream created: capacity=%zu, empty=%d\n", stream.capacity(), stream.empty());
    print_separator();

    // Add events to stream
    printf("Adding events to stream:\n");

    uint64_t seq = 0;
    event_stream_result result;

    result = stream.push(counter_event::make_increment(5), 1000, &seq);
    printf("  [%lu] Increment by 5 - result=%d\n", seq, static_cast<int>(result));

    result = stream.push(counter_event::make_increment(3), 2000, &seq);
    printf("  [%lu] Increment by 3 - result=%d\n", seq, static_cast<int>(result));

    result = stream.push(counter_event::make_decrement(2), 3000, &seq);
    printf("  [%lu] Decrement by 2 - result=%d\n", seq, static_cast<int>(result));

    result = stream.push(counter_event::make_increment(10), 4000, &seq);
    printf("  [%lu] Increment by 10 - result=%d\n", seq, static_cast<int>(result));

    result = stream.push(counter_event::make_reset(100), 5000, &seq);
    printf("  [%lu] Reset to 100 - result=%d\n", seq, static_cast<int>(result));

    result = stream.push(counter_event::make_decrement(25), 6000, &seq);
    printf("  [%lu] Decrement by 25 - result=%d\n", seq, static_cast<int>(result));

    printf("\nStream statistics:\n");
    const event_stream_stats& stats = stream.stats();
    printf("  Total events: %zu\n", stats.total_events);
    printf("  Current events: %zu\n", stats.current_events);
    printf("  Sequence range: [%lu, %lu]\n", stats.min_sequence, stats.max_sequence);
    printf("  Memory used: %zu bytes\n", stats.bytes_allocated);
    print_separator();

    // First replay - apply all events
    printf("First replay: Applying all events...\n");
    event_stream_result stream_result;
    state_snapshot<counter_state> final_state1 =
        stream.replay(initial_state, handle_counter_event, stream_result);

    printf("Replay result: %d\n", static_cast<int>(stream_result));
    print_state("Final state (replay 1)", final_state1);
    print_separator();

    // Second replay - should produce IDENTICAL state (determinism test)
    printf("Second replay: Applying same events...\n");
    state_snapshot<counter_state> final_state2 =
        stream.replay(initial_state, handle_counter_event, stream_result);

    printf("Replay result: %d\n", static_cast<int>(stream_result));
    print_state("Final state (replay 2)", final_state2);
    print_separator();

    // Verify determinism
    printf("Determinism verification:\n");
    bool states_equal = (final_state1.data() == final_state2.data());
    bool versions_equal = (final_state1.version() == final_state2.version());
    printf("  States equal: %s\n", states_equal ? "YES" : "NO");
    printf("  Versions equal: %s\n", versions_equal ? "YES" : "NO");
    printf("  Deterministic: %s\n", (states_equal && versions_equal) ? "✓ PASSED" : "✗ FAILED");
    print_separator();

    // Demonstrate event iteration
    printf("Iterating over events:\n");
    size_t event_num = 0;
    stream.for_each([&event_num](const state_event<counter_event>& event) {
        printf("  Event #%zu: seq=%lu, timestamp=%lu ns, type=%d\n", event_num,
               event.metadata.sequence, event.metadata.timestamp_ns,
               static_cast<int>(event.data.type));
        event_num++;
        return true; // Continue iteration
    });
    print_separator();

    // Third replay with state_transition wrapper
    printf("Third replay: Using state_transition wrapper...\n");
    state_transition<counter_state, counter_event> transition(handle_counter_event);
    state_snapshot<counter_state> final_state3 =
        stream.replay(initial_state, transition, stream_result);

    printf("Replay result: %d\n", static_cast<int>(stream_result));
    print_state("Final state (replay 3)", final_state3);

    // Verify all three replays produce identical results
    bool all_equal = (final_state1.data() == final_state2.data()) &&
                     (final_state2.data() == final_state3.data());
    printf("\nAll three replays identical: %s\n", all_equal ? "✓ YES" : "✗ NO");
    print_separator();

    // Demonstrate manual event processing
    printf("Manual event processing (without replay):\n");
    state_snapshot<counter_state> manual_state = initial_state;
    for (size_t i = 0; i < stream.count(); ++i) {
        const state_event<counter_event>* event = stream.get(i);
        if (event != nullptr) {
            transition_result trans_result;
            manual_state = handle_counter_event(manual_state, *event, trans_result);
            printf("  After event %zu: value=%d\n", i, manual_state.data().value);
        }
    }
    print_state("Manual final state", manual_state);

    bool manual_equals_replay = (manual_state.data() == final_state1.data());
    printf("Manual processing matches replay: %s\n", manual_equals_replay ? "✓ YES" : "✗ NO");
    print_separator();

    printf("Demo completed successfully!\n");
    printf("\nKey properties demonstrated:\n");
    printf("  ✓ Events are stored in deterministic order (by sequence)\n");
    printf("  ✓ Replaying event stream produces identical state\n");
    printf("  ✓ No callbacks - only explicit iteration and replay\n");
    printf("  ✓ Event processing is explicit and controllable\n");

    return 0;
}
