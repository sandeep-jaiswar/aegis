#pragma once

#include <cstdint>

namespace aegis::core::events {

// Input event types - deterministic representation of all user input
// These are OS-agnostic and can be replayed deterministically
enum class input_event_type : uint8_t {
    keyboard_down = 0,
    keyboard_up = 1,
    mouse_move = 2,
    mouse_button_down = 3,
    mouse_button_up = 4,
    mouse_scroll = 5,
    touch_begin = 6,
    touch_move = 7,
    touch_end = 8,
    touch_cancel = 9
};

// Keyboard key codes (platform-agnostic)
// NOLINTNEXTLINE(performance-enum-size) - uint16_t for future expansion
enum class key_code : uint16_t {
    unknown = 0,
    // Letters
    a = 65,
    b = 66,
    c = 67,
    d = 68,
    e = 69,
    f = 70,
    g = 71,
    h = 72,
    i = 73,
    j = 74,
    k = 75,
    l = 76,
    m = 77,
    n = 78,
    o = 79,
    p = 80,
    q = 81,
    r = 82,
    s = 83,
    t = 84,
    u = 85,
    v = 86,
    w = 87,
    x = 88,
    y = 89,
    z = 90,
    // Numbers
    num_0 = 48,
    num_1 = 49,
    num_2 = 50,
    num_3 = 51,
    num_4 = 52,
    num_5 = 53,
    num_6 = 54,
    num_7 = 55,
    num_8 = 56,
    num_9 = 57,
    // Special keys
    space = 32,
    enter = 13,
    tab = 9,
    backspace = 8,
    escape = 27,
    // Arrow keys
    left = 37,
    up = 38,
    right = 39,
    down = 40,
    // Modifiers
    shift = 16,
    control = 17,
    alt = 18,
    meta = 91 // Command/Windows key
};

// Mouse button codes
enum class mouse_button : uint8_t { left = 0, middle = 1, right = 2, button_4 = 3, button_5 = 4 };

// Modifier key state flags
struct key_modifiers {
    bool shift : 1;
    bool control : 1;
    bool alt : 1;
    bool meta : 1;
    // NOLINTNEXTLINE(readability-identifier-naming) - padding for alignment
    uint8_t _padding : 4;

    [[nodiscard]] constexpr bool any() const noexcept {
        return shift || control || alt || meta;
    }

    [[nodiscard]] constexpr bool none() const noexcept {
        return !any();
    }
};

// Keyboard event data
struct keyboard_event {
    key_code key{key_code::unknown};
    key_modifiers modifiers{};
    // NOLINTNEXTLINE(readability-identifier-naming) - padding for alignment
    uint16_t _padding{0};
};

// Mouse event data
struct mouse_event {
    float x{0.0F};             // X coordinate (screen space)
    float y{0.0F};             // Y coordinate (screen space)
    float delta_x{0.0F};       // Movement delta (for mouse_move)
    float delta_y{0.0F};       // Movement delta (for mouse_move)
    mouse_button button{};     // Button (for button events)
    key_modifiers modifiers{}; // Modifier keys held
    // NOLINTNEXTLINE(readability-identifier-naming) - padding for alignment
    uint8_t _padding[2]{};
};

// Mouse scroll event data
struct scroll_event {
    float delta_x{0.0F};       // Horizontal scroll delta
    float delta_y{0.0F};       // Vertical scroll delta
    key_modifiers modifiers{}; // Modifier keys held
    // NOLINTNEXTLINE(readability-identifier-naming) - padding for alignment
    uint8_t _padding[3]{};
};

// Touch event data (single touch point)
struct touch_event {
    uint32_t touch_id{0}; // Unique touch identifier
    float x{0.0F};        // X coordinate (screen space)
    float y{0.0F};        // Y coordinate (screen space)
    float pressure{1.0F}; // Pressure (0.0 to 1.0)
    float radius_x{1.0F}; // Touch area radius X
    float radius_y{1.0F}; // Touch area radius Y
    float rotation{0.0F}; // Touch rotation angle
    // NOLINTNEXTLINE(readability-identifier-naming) - padding for alignment
    uint32_t _padding{0};
};

// Generic input event wrapper
// Contains type discriminator and event data
struct input_event {
    input_event_type type{};
    // NOLINTNEXTLINE(readability-identifier-naming) - padding for alignment
    uint8_t _padding[3]{};
    uint64_t timestamp_ns{0}; // Event timestamp for ordering

    // Event data (union to save space)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) - union needed for compact storage
    union {
        keyboard_event keyboard;
        mouse_event mouse;
        scroll_event scroll;
        touch_event touch;
    };

    // Constructors for different event types
    constexpr input_event() noexcept : keyboard{} {
    }

    static constexpr input_event make_keyboard(key_code key, key_modifiers mods,
                                               uint64_t timestamp) noexcept {
        input_event evt;
        evt.type = input_event_type::keyboard_down;
        evt.timestamp_ns = timestamp;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.keyboard.key = key;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.keyboard.modifiers = mods;
        return evt;
    }

    static constexpr input_event make_mouse_move(float x, float y, float dx, float dy,
                                                 key_modifiers mods, uint64_t timestamp) noexcept {
        input_event evt;
        evt.type = input_event_type::mouse_move;
        evt.timestamp_ns = timestamp;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.mouse.x = x;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.mouse.y = y;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.mouse.delta_x = dx;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.mouse.delta_y = dy;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.mouse.modifiers = mods;
        return evt;
    }

    static constexpr input_event make_mouse_button(input_event_type type, mouse_button button,
                                                   float x, float y, key_modifiers mods,
                                                   uint64_t timestamp) noexcept {
        input_event evt;
        evt.type = type;
        evt.timestamp_ns = timestamp;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.mouse.x = x;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.mouse.y = y;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.mouse.button = button;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.mouse.modifiers = mods;
        return evt;
    }

    static constexpr input_event make_scroll(float dx, float dy, key_modifiers mods,
                                             uint64_t timestamp) noexcept {
        input_event evt;
        evt.type = input_event_type::mouse_scroll;
        evt.timestamp_ns = timestamp;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.scroll.delta_x = dx;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.scroll.delta_y = dy;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.scroll.modifiers = mods;
        return evt;
    }

    static constexpr input_event make_touch(input_event_type type, uint32_t touch_id, float x,
                                            float y, float pressure, uint64_t timestamp) noexcept {
        input_event evt;
        evt.type = type;
        evt.timestamp_ns = timestamp;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.touch.touch_id = touch_id;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.touch.x = x;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.touch.y = y;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        evt.touch.pressure = pressure;
        return evt;
    }
};

// Verify input_event is suitable for fast replay
static_assert(sizeof(input_event) <= 64, "input_event should be compact for cache efficiency");

} // namespace aegis::core::events
