#include "core/frame/frame_lifecycle.hpp"

namespace aegis::core::frame {

// frame_context implementation

bool frame_context::is_valid_transition(frame_phase next_phase) const noexcept {
    // Frame phases must execute in strict order:
    // idle -> begin -> apply_events -> update_state -> compute_layout ->
    // build_scene -> diff_scene -> end -> idle

    const auto current = static_cast<uint8_t>(current_phase);
    const auto next = static_cast<uint8_t>(next_phase);

    // Special case: idle can transition to begin
    if (current_phase == frame_phase::idle && next_phase == frame_phase::begin) {
        return true;
    }

    // Special case: end can transition to idle (for reset)
    if (current_phase == frame_phase::end && next_phase == frame_phase::idle) {
        return true;
    }

    // Normal progression: next phase must be exactly current + 1
    return next == current + 1;
}

void frame_context::update_phase_timing(uint64_t current_time_ns) noexcept {
    if (phase_start_time_ns == 0) {
        return;
    }

    const uint64_t phase_duration = current_time_ns - phase_start_time_ns;

    // Update timing for the phase we're leaving
    switch (current_phase) {
        case frame_phase::begin:
            stats.begin_time_ns = phase_duration;
            break;
        case frame_phase::apply_events:
            stats.apply_events_time_ns = phase_duration;
            break;
        case frame_phase::update_state:
            stats.update_state_time_ns = phase_duration;
            break;
        case frame_phase::compute_layout:
            stats.compute_layout_time_ns = phase_duration;
            break;
        case frame_phase::build_scene:
            stats.build_scene_time_ns = phase_duration;
            break;
        case frame_phase::diff_scene:
            stats.diff_scene_time_ns = phase_duration;
            break;
        case frame_phase::end:
            stats.end_time_ns = phase_duration;
            // Calculate total frame time from frame start to now
            stats.total_time_ns = current_time_ns - stats.frame_start_timestamp_ns;
            break;
        case frame_phase::idle:
            break;
    }
}

frame_result frame_context::begin_frame(uint64_t timestamp_ns) noexcept {
    if (!is_valid_transition(frame_phase::begin)) {
        return frame_result::invalid_phase_transition;
    }

    current_phase = frame_phase::begin;
    phase_start_time_ns = timestamp_ns;
    stats.frame_number++;
    stats.frame_start_timestamp_ns = timestamp_ns; // Store frame start time

    return frame_result::success;
}

frame_result frame_context::apply_events() noexcept {
    if (!is_valid_transition(frame_phase::apply_events)) {
        return frame_result::invalid_phase_transition;
    }

    // NOTE: Placeholder timing - In production, this would use a platform-specific
    // high-resolution timer provided by the runtime layer (e.g., std::chrono or OS API)
    // The +1 ensures phases have non-zero duration for testing
    const uint64_t current_time = phase_start_time_ns + 1;

    update_phase_timing(current_time);
    current_phase = frame_phase::apply_events;
    phase_start_time_ns = current_time;

    return frame_result::success;
}

frame_result frame_context::update_state() noexcept {
    if (!is_valid_transition(frame_phase::update_state)) {
        return frame_result::invalid_phase_transition;
    }

    // NOTE: Placeholder timing - see apply_events() for explanation
    const uint64_t current_time = phase_start_time_ns + 1;
    update_phase_timing(current_time);
    current_phase = frame_phase::update_state;
    phase_start_time_ns = current_time;

    return frame_result::success;
}

frame_result frame_context::compute_layout() noexcept {
    if (!is_valid_transition(frame_phase::compute_layout)) {
        return frame_result::invalid_phase_transition;
    }

    // NOTE: Placeholder timing - see apply_events() for explanation
    const uint64_t current_time = phase_start_time_ns + 1;
    update_phase_timing(current_time);
    current_phase = frame_phase::compute_layout;
    phase_start_time_ns = current_time;

    return frame_result::success;
}

frame_result frame_context::build_scene() noexcept {
    if (!is_valid_transition(frame_phase::build_scene)) {
        return frame_result::invalid_phase_transition;
    }

    // NOTE: Placeholder timing - see apply_events() for explanation
    const uint64_t current_time = phase_start_time_ns + 1;
    update_phase_timing(current_time);
    current_phase = frame_phase::build_scene;
    phase_start_time_ns = current_time;

    return frame_result::success;
}

frame_result frame_context::diff_scene() noexcept {
    if (!is_valid_transition(frame_phase::diff_scene)) {
        return frame_result::invalid_phase_transition;
    }

    // NOTE: Placeholder timing - see apply_events() for explanation
    const uint64_t current_time = phase_start_time_ns + 1;
    update_phase_timing(current_time);
    current_phase = frame_phase::diff_scene;
    phase_start_time_ns = current_time;

    return frame_result::success;
}

frame_result frame_context::end_frame() noexcept {
    if (!is_valid_transition(frame_phase::end)) {
        return frame_result::invalid_phase_transition;
    }

    // NOTE: Placeholder timing - see apply_events() for explanation
    const uint64_t current_time = phase_start_time_ns + 1;
    update_phase_timing(current_time);
    current_phase = frame_phase::end;

    // Capture frame allocator statistics before reset
    if (frame_alloc != nullptr) {
        stats.frame_allocations = frame_alloc->get_allocation_count();
        stats.frame_deallocations = frame_alloc->get_deallocation_count();
        
        // Update memory statistics from frame allocator
        stats.bytes_allocated += frame_alloc->bytes_allocated();
        stats.bytes_freed += frame_alloc->bytes_freed();
        
        const size_t current_used = stats.bytes_allocated - stats.bytes_freed;
        if (current_used > stats.peak_memory_used) {
            stats.peak_memory_used = current_used;
        }
        
        // Reset frame allocator - O(1) operation that invalidates all frame allocations
        frame_alloc->reset();
    }

    // Check time budget
    if (!is_within_budget()) {
        return frame_result::exceeded_time_budget;
    }

    return frame_result::success;
}

// frame_executor implementation

frame_result frame_executor::execute_begin_frame(uint64_t timestamp_ns,
                                                 frame_context& ctx) noexcept {
    const frame_result result = ctx.begin_frame(timestamp_ns);
    if (result != frame_result::success) {
        return result;
    }

    on_begin_frame(ctx);
    return frame_result::success;
}

frame_result frame_executor::execute_apply_events(frame_context& ctx) noexcept {
    const frame_result result = ctx.apply_events();
    if (result != frame_result::success) {
        return result;
    }

    on_apply_events(ctx);
    return frame_result::success;
}

frame_result frame_executor::execute_update_state(frame_context& ctx) noexcept {
    const frame_result result = ctx.update_state();
    if (result != frame_result::success) {
        return result;
    }

    on_update_state(ctx);
    return frame_result::success;
}

frame_result frame_executor::execute_compute_layout(frame_context& ctx) noexcept {
    const frame_result result = ctx.compute_layout();
    if (result != frame_result::success) {
        return result;
    }

    on_compute_layout(ctx);
    return frame_result::success;
}

frame_result frame_executor::execute_build_scene(frame_context& ctx) noexcept {
    const frame_result result = ctx.build_scene();
    if (result != frame_result::success) {
        return result;
    }

    on_build_scene(ctx);
    return frame_result::success;
}

frame_result frame_executor::execute_diff_scene(frame_context& ctx) noexcept {
    const frame_result result = ctx.diff_scene();
    if (result != frame_result::success) {
        return result;
    }

    on_diff_scene(ctx);
    return frame_result::success;
}

frame_result frame_executor::execute_end_frame(frame_context& ctx) noexcept {
    const frame_result result = ctx.end_frame();
    if (result != frame_result::success) {
        return result;
    }

    on_end_frame(ctx);
    return frame_result::success;
}

frame_result frame_executor::execute_frame(uint64_t timestamp_ns, frame_context& ctx) noexcept {
    // Execute all frame phases in strict order
    frame_result result = execute_begin_frame(timestamp_ns, ctx);
    if (result != frame_result::success) {
        return result;
    }

    result = execute_apply_events(ctx);
    if (result != frame_result::success) {
        return result;
    }

    result = execute_update_state(ctx);
    if (result != frame_result::success) {
        return result;
    }

    result = execute_compute_layout(ctx);
    if (result != frame_result::success) {
        return result;
    }

    result = execute_build_scene(ctx);
    if (result != frame_result::success) {
        return result;
    }

    result = execute_diff_scene(ctx);
    if (result != frame_result::success) {
        return result;
    }

    result = execute_end_frame(ctx);
    return result;
}

} // namespace aegis::core::frame
