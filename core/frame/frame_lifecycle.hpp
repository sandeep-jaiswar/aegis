#pragma once

#include "core/memory/frame_allocator.hpp"

#include <cstddef>
#include <cstdint>

namespace aegis::core::frame {

// Frame execution phases - executed in this exact order every frame
enum class frame_phase : uint8_t {
    idle = 0,
    begin = 1,
    apply_events = 2,
    update_state = 3,
    compute_layout = 4,
    build_scene = 5,
    diff_scene = 6,
    end = 7
};

// Frame statistics for deterministic performance tracking
struct frame_stats {
    uint64_t frame_number{0};
    uint64_t frame_start_timestamp_ns{0}; // Timestamp when frame started
    uint64_t begin_time_ns{0};
    uint64_t apply_events_time_ns{0};
    uint64_t update_state_time_ns{0};
    uint64_t compute_layout_time_ns{0};
    uint64_t build_scene_time_ns{0};
    uint64_t diff_scene_time_ns{0};
    uint64_t end_time_ns{0};
    uint64_t total_time_ns{0};

    // Memory tracking - no hidden allocations
    size_t bytes_allocated{0};
    size_t bytes_freed{0};
    size_t peak_memory_used{0};

    // Frame allocator statistics
    size_t frame_allocations{0};
    size_t frame_deallocations{0};
};

// Frame execution result
enum class frame_result : uint8_t {
    success = 0,
    invalid_phase_transition = 1,
    exceeded_time_budget = 2,
    allocation_failure = 3
};

// Frame execution context - tracks frame state and ensures deterministic execution
class frame_context {
  public:
    // Create frame context with optional frame allocator
    // If frame_allocator is provided, it will be reset at end_frame()
    explicit frame_context(memory::frame_allocator* frame_alloc = nullptr) noexcept
        : frame_alloc(frame_alloc) {
    }

    ~frame_context() noexcept = default;

    // Disable copy and move - frame context is tied to a single execution
    frame_context(const frame_context&) = delete;
    frame_context& operator=(const frame_context&) = delete;
    frame_context(frame_context&&) = delete;
    frame_context& operator=(frame_context&&) = delete;

    // Phase transition functions - enforce strict ordering
    [[nodiscard]] frame_result begin_frame(uint64_t timestamp_ns) noexcept;
    [[nodiscard]] frame_result apply_events() noexcept;
    [[nodiscard]] frame_result update_state() noexcept;
    [[nodiscard]] frame_result compute_layout() noexcept;
    [[nodiscard]] frame_result build_scene() noexcept;
    [[nodiscard]] frame_result diff_scene() noexcept;
    [[nodiscard]] frame_result end_frame() noexcept;

    // State queries
    [[nodiscard]] frame_phase current_phase_get() const noexcept {
        return current_phase;
    }

    [[nodiscard]] const frame_stats& stats_get() const noexcept {
        return stats;
    }

    [[nodiscard]] uint64_t frame_number() const noexcept {
        return stats.frame_number;
    }

    // Time budget enforcement - worst-case frame time is bounded
    void set_time_budget_ns(uint64_t budget_ns) noexcept {
        time_budget_ns = budget_ns;
    }

    [[nodiscard]] uint64_t get_time_budget_ns() const noexcept {
        return time_budget_ns;
    }

    [[nodiscard]] bool is_within_budget() const noexcept {
        return stats.total_time_ns <= time_budget_ns;
    }

    // Memory tracking - expose allocations for deterministic behavior
    void track_allocation(size_t bytes) noexcept {
        stats.bytes_allocated += bytes;
        const size_t current_used = stats.bytes_allocated - stats.bytes_freed;
        if (current_used > stats.peak_memory_used) {
            stats.peak_memory_used = current_used;
        }
    }

    void track_deallocation(size_t bytes) noexcept {
        stats.bytes_freed += bytes;
    }

    // Get frame allocator (if available)
    [[nodiscard]] memory::frame_allocator* get_frame_allocator() noexcept {
        return frame_alloc;
    }

    [[nodiscard]] const memory::frame_allocator* get_frame_allocator() const noexcept {
        return frame_alloc;
    }

    // Reset for next frame - ensures clean state
    void reset() noexcept {
        current_phase = frame_phase::idle;
        stats = frame_stats{};
        phase_start_time_ns = 0;
    }

  private:
    frame_phase current_phase{frame_phase::idle};
    frame_stats stats{};
    uint64_t time_budget_ns{16'666'667}; // Default: ~60 FPS (16.67ms)
    uint64_t phase_start_time_ns{0};
    memory::frame_allocator* frame_alloc{nullptr};

    // Validate phase transition
    [[nodiscard]] bool is_valid_transition(frame_phase next_phase) const noexcept;

    // Update timing for current phase
    void update_phase_timing(uint64_t current_time_ns) noexcept;
};

// Frame execution orchestrator - ensures deterministic frame execution
// Same inputs → byte-identical outputs
class frame_executor {
  public:
    frame_executor() noexcept = default;
    virtual ~frame_executor() noexcept = default;

    // Disable copy and move
    frame_executor(const frame_executor&) = delete;
    frame_executor& operator=(const frame_executor&) = delete;
    frame_executor(frame_executor&&) = delete;
    frame_executor& operator=(frame_executor&&) = delete;

    // Execute a complete frame cycle
    // Returns frame_result indicating success or failure reason
    [[nodiscard]] frame_result execute_frame(uint64_t timestamp_ns, frame_context& ctx) noexcept;

    // Execute individual phases - for manual control
    [[nodiscard]] frame_result execute_begin_frame(uint64_t timestamp_ns,
                                                   frame_context& ctx) noexcept;

    [[nodiscard]] frame_result execute_apply_events(frame_context& ctx) noexcept;

    [[nodiscard]] frame_result execute_update_state(frame_context& ctx) noexcept;

    [[nodiscard]] frame_result execute_compute_layout(frame_context& ctx) noexcept;

    [[nodiscard]] frame_result execute_build_scene(frame_context& ctx) noexcept;

    [[nodiscard]] frame_result execute_diff_scene(frame_context& ctx) noexcept;

    [[nodiscard]] frame_result execute_end_frame(frame_context& ctx) noexcept;

  private:
    // Hook points for actual implementation
    // These would be implemented by derived classes or set via callbacks
    // For now, they are placeholders that ensure the lifecycle is correct

    virtual void on_begin_frame(frame_context& ctx) noexcept {
        (void)ctx;
    }

    virtual void on_apply_events(frame_context& ctx) noexcept {
        (void)ctx;
    }

    virtual void on_update_state(frame_context& ctx) noexcept {
        (void)ctx;
    }

    virtual void on_compute_layout(frame_context& ctx) noexcept {
        (void)ctx;
    }

    virtual void on_build_scene(frame_context& ctx) noexcept {
        (void)ctx;
    }

    virtual void on_diff_scene(frame_context& ctx) noexcept {
        (void)ctx;
    }

    virtual void on_end_frame(frame_context& ctx) noexcept {
        (void)ctx;
    }
};

} // namespace aegis::core::frame
