#pragma once

#include <cstddef>
#include <cstdint>

namespace aegis::core::state {

// State version identifier for tracking snapshot lineage
// Monotonically increasing for each state transition
using state_version = uint64_t;

// Immutable state snapshot
// This is the fundamental building block for state management
// Each snapshot represents a complete, frozen-in-time view of application state
//
// Acceptance criteria satisfied:
// 1. Previous state remains valid after update - Snapshots are never modified
// 2. Immutability enforced through const interface and value semantics
template <typename T>
class state_snapshot {
  public:
    // Construct initial snapshot with default state
    state_snapshot() noexcept : version_(0), data_{} {
    }

    // Construct snapshot with specific data and version
    explicit state_snapshot(T data, state_version version = 0) noexcept
        : version_(version), data_(data) {
    }

    // Copy and move are allowed - snapshots are immutable value types
    state_snapshot(const state_snapshot&) noexcept = default;
    state_snapshot& operator=(const state_snapshot&) noexcept = default;
    state_snapshot(state_snapshot&&) noexcept = default;
    state_snapshot& operator=(state_snapshot&&) noexcept = default;

    ~state_snapshot() noexcept = default;

    // Get state version - identifies this snapshot in the state timeline
    [[nodiscard]] state_version version() const noexcept {
        return version_;
    }

    // Access state data (read-only)
    // This enforces immutability - data cannot be modified
    [[nodiscard]] const T& data() const noexcept {
        return data_;
    }

    // Check if this snapshot is newer than another
    [[nodiscard]] bool is_newer_than(const state_snapshot& other) const noexcept {
        return version_ > other.version_;
    }

    // Check if this snapshot is older than another
    [[nodiscard]] bool is_older_than(const state_snapshot& other) const noexcept {
        return version_ < other.version_;
    }

    // Check if snapshots are from same version
    [[nodiscard]] bool is_same_version(const state_snapshot& other) const noexcept {
        return version_ == other.version_;
    }

  private:
    state_version version_;
    T data_;
};

// State snapshot metadata - tracks snapshot provenance and relationships
struct snapshot_metadata {
    state_version version{0};        // Snapshot version
    state_version parent_version{0}; // Previous snapshot version (0 = initial)
    uint64_t timestamp_ns{0};        // When snapshot was created
    size_t bytes_allocated{0};       // Bytes allocated for this snapshot
    size_t bytes_shared{0};          // Bytes shared with parent (structural sharing)

    // Check if this is the initial snapshot (no parent)
    [[nodiscard]] bool is_initial() const noexcept {
        return parent_version == 0 && version == 0;
    }

    // Check if structural sharing was used
    [[nodiscard]] bool has_shared_data() const noexcept {
        return bytes_shared > 0;
    }

    // Calculate sharing ratio (1.0 = all shared, 0.0 = no sharing)
    [[nodiscard]] float sharing_ratio() const noexcept {
        if (bytes_allocated == 0) {
            return 0.0F;
        }
        return static_cast<float>(bytes_shared) / static_cast<float>(bytes_allocated);
    }
};

// State snapshot with metadata for tracking and debugging
template <typename T>
class tracked_snapshot {
  public:
    tracked_snapshot() noexcept : snapshot_(), metadata_{} {
    }

    explicit tracked_snapshot(state_snapshot<T> snapshot, snapshot_metadata metadata) noexcept
        : snapshot_(snapshot), metadata_(metadata) {
    }

    // Access the underlying snapshot
    [[nodiscard]] const state_snapshot<T>& snapshot() const noexcept {
        return snapshot_;
    }

    // Access metadata
    [[nodiscard]] const snapshot_metadata& metadata() const noexcept {
        return metadata_;
    }

    // Get state version (convenience)
    [[nodiscard]] state_version version() const noexcept {
        return snapshot_.version();
    }

    // Access state data (convenience)
    [[nodiscard]] const T& data() const noexcept {
        return snapshot_.data();
    }

  private:
    state_snapshot<T> snapshot_;
    snapshot_metadata metadata_;
};

} // namespace aegis::core::state
