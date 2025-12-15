#pragma once

#include <cstddef>
#include <cstdint>

namespace aegis::core::layout {

// Layout dimension types - explicit, no implicit conversions
struct dimension {
    float value{0.0F};

    constexpr dimension() noexcept = default;
    constexpr explicit dimension(float v) noexcept : value(v) {
    }

    constexpr dimension operator+(dimension other) const noexcept {
        return dimension{value + other.value};
    }

    constexpr dimension operator-(dimension other) const noexcept {
        return dimension{value - other.value};
    }

    constexpr bool operator<(dimension other) const noexcept {
        return value < other.value;
    }

    constexpr bool operator>(dimension other) const noexcept {
        return value > other.value;
    }
};

// Size constraint - specifies how a box should be sized
enum class size_constraint : uint8_t {
    fixed = 0,  // Fixed size (width/height exactly specified)
    min = 1,    // Minimum size (can grow)
    max = 2,    // Maximum size (can shrink)
    fill = 3,   // Fill available space
    content = 4 // Size based on content
};

// Alignment within parent
enum class alignment : uint8_t {
    start = 0,  // Align to start (left/top)
    center = 1, // Center
    end = 2,    // Align to end (right/bottom)
    stretch = 3 // Stretch to fill
};

// Layout direction
enum class direction : uint8_t { horizontal = 0, vertical = 1 };

// Box constraints - defines how a box can be sized
struct box_constraints {
    dimension min_width{0.0F};
    dimension max_width{10000.0F}; // Reasonable default max
    dimension min_height{0.0F};
    dimension max_height{10000.0F};

    size_constraint width_constraint{size_constraint::content};
    size_constraint height_constraint{size_constraint::content};

    constexpr box_constraints() noexcept = default;

    // Constrain a size to fit within these constraints
    [[nodiscard]] constexpr dimension constrain_width(dimension width) const noexcept {
        if (width < min_width) {
            return min_width;
        }
        if (width > max_width) {
            return max_width;
        }
        return width;
    }

    [[nodiscard]] constexpr dimension constrain_height(dimension height) const noexcept {
        if (height < min_height) {
            return min_height;
        }
        if (height > max_height) {
            return max_height;
        }
        return height;
    }
};

// Size in 2D
struct size {
    dimension width{0.0F};
    dimension height{0.0F};

    constexpr size() noexcept = default;
    constexpr size(dimension w, dimension h) noexcept : width(w), height(h) {
    }
};

// Position in 2D
struct position {
    dimension x{0.0F};
    dimension y{0.0F};

    constexpr position() noexcept = default;
    constexpr position(dimension x_val, dimension y_val) noexcept : x(x_val), y(y_val) {
    }
};

// Rectangle - position + size
struct rect {
    position pos;
    size sz;

    constexpr rect() noexcept = default;
    constexpr rect(position p, size s) noexcept : pos(p), sz(s) {
    }
    constexpr rect(dimension x, dimension y, dimension w, dimension h) noexcept
        : pos(x, y), sz(w, h) {
    }
};

// Padding around content
struct padding {
    dimension left{0.0F};
    dimension right{0.0F};
    dimension top{0.0F};
    dimension bottom{0.0F};

    constexpr padding() noexcept = default;
    constexpr explicit padding(dimension all) noexcept
        : left(all), right(all), top(all), bottom(all) {
    }
    constexpr padding(dimension l, dimension r, dimension t, dimension b) noexcept
        : left(l), right(r), top(t), bottom(b) {
    }

    [[nodiscard]] constexpr dimension horizontal() const noexcept {
        return left + right;
    }

    [[nodiscard]] constexpr dimension vertical() const noexcept {
        return top + bottom;
    }
};

} // namespace aegis::core::layout
