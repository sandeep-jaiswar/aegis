#include "core/events/input_event.hpp"

#include "runtime/platform/null_platform.hpp"
#include "runtime/platform/platform_adapter.hpp"

#include <cstdio>

using namespace aegis::core::events;
using namespace aegis::runtime::platform;

// Event handler callback
static bool handle_event(const input_event& event, void* /*user_data*/) noexcept {
    printf("Event received: ");

    switch (event.type) {
        case input_event_type::keyboard_down:
            printf("KEYBOARD_DOWN - key=%u, timestamp=%lu\n",
                   static_cast<uint16_t>(event.keyboard.key), event.timestamp_ns);
            break;
        case input_event_type::keyboard_up:
            printf("KEYBOARD_UP - key=%u, timestamp=%lu\n",
                   static_cast<uint16_t>(event.keyboard.key), event.timestamp_ns);
            break;
        case input_event_type::mouse_move:
            printf("MOUSE_MOVE - pos=(%.1f, %.1f), delta=(%.1f, %.1f), timestamp=%lu\n",
                   event.mouse.x, event.mouse.y, event.mouse.delta_x, event.mouse.delta_y,
                   event.timestamp_ns);
            break;
        case input_event_type::mouse_button_down:
            printf("MOUSE_DOWN - button=%u, pos=(%.1f, %.1f), timestamp=%lu\n",
                   static_cast<uint8_t>(event.mouse.button), event.mouse.x, event.mouse.y,
                   event.timestamp_ns);
            break;
        case input_event_type::mouse_button_up:
            printf("MOUSE_UP - button=%u, pos=(%.1f, %.1f), timestamp=%lu\n",
                   static_cast<uint8_t>(event.mouse.button), event.mouse.x, event.mouse.y,
                   event.timestamp_ns);
            break;
        case input_event_type::mouse_scroll:
            printf("MOUSE_SCROLL - delta=(%.1f, %.1f), timestamp=%lu\n", event.scroll.delta_x,
                   event.scroll.delta_y, event.timestamp_ns);
            break;
        default:
            printf("UNKNOWN - type=%u, timestamp=%lu\n", static_cast<uint8_t>(event.type),
                   event.timestamp_ns);
            break;
    }

    return true;
}

int main() {
    printf("=== Aegis Platform Adapter Demo ===\n\n");

    // Create null platform adapter for testing
    null_platform_adapter platform;

    printf("1. Initializing platform adapter...\n");
    if (!platform.initialize()) {
        printf("Failed to initialize platform!\n");
        return 1;
    }
    printf("   Platform initialized successfully\n\n");

    // Set event callback
    printf("2. Registering event callback...\n");
    platform.set_event_callback(handle_event, nullptr);
    printf("   Callback registered\n\n");

    // Inject some test events
    printf("3. Injecting test events...\n\n");

    // Keyboard event
    platform.set_time(1000000); // 1ms
    auto kbd_event = input_event::make_keyboard(
        key_code::a, key_modifiers{false, false, false, false, 0}, 1000000);
    platform.inject_event(kbd_event);

    // Mouse move event
    platform.set_time(2000000); // 2ms
    auto mouse_move = input_event::make_mouse_move(
        100.0F, 200.0F, 5.0F, 10.0F, key_modifiers{false, false, false, false, 0}, 2000000);
    platform.inject_event(mouse_move);

    // Mouse button event
    platform.set_time(3000000); // 3ms
    auto mouse_down = input_event::make_mouse_button(
        input_event_type::mouse_button_down, mouse_button::left, 100.0F, 200.0F,
        key_modifiers{false, false, false, false, 0}, 3000000);
    platform.inject_event(mouse_down);

    // Mouse scroll event
    platform.set_time(4000000); // 4ms
    auto scroll = input_event::make_scroll(0.0F, -10.0F,
                                           key_modifiers{false, false, false, false, 0}, 4000000);
    platform.inject_event(scroll);

    printf("\n4. Testing event replay...\n");
    event_replay_buffer replay_buffer(100);

    // Record events
    printf("   Recording events to replay buffer...\n");
    (void)replay_buffer.push(kbd_event);
    (void)replay_buffer.push(mouse_move);
    (void)replay_buffer.push(mouse_down);
    (void)replay_buffer.push(scroll);
    printf("   Recorded %zu events\n\n", replay_buffer.count());

    // Replay events
    printf("   Replaying events:\n");
    replay_buffer.replay_all(handle_event, nullptr);

    printf("\n5. Shutting down platform...\n");
    platform.shutdown();
    printf("   Platform shutdown complete\n\n");

    printf("=== Demo completed successfully ===\n");
    return 0;
}
