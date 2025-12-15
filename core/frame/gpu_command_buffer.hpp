#pragma once

#include "core/frame/diff_engine.hpp"
#include "core/frame/gpu_command.hpp"
#include "core/frame/scene_graph.hpp"
#include "core/memory/allocator.hpp"

#include <cstddef>
#include <cstdint>

namespace aegis::core::frame {

// GPU command buffer configuration
struct gpu_command_buffer_config {
    uint32_t max_commands{4096};       // Maximum GPU commands per frame
    uint32_t max_buffer_data_bytes{65536}; // Maximum buffer update data (64KB)
};

// GPU command buffer result codes
enum class gpu_command_result : uint8_t {
    success = 0,
    out_of_memory = 1,
    command_limit_exceeded = 2,
    invalid_input = 3,
    buffer_data_limit_exceeded = 4
};

// GPU command buffer statistics
struct gpu_command_stats {
    uint32_t total_commands{0};
    uint32_t buffer_updates{0};
    uint32_t draw_calls{0};
    uint32_t batched_draws{0};
    uint32_t pipeline_changes{0};
    uint32_t sync_points{0};
    uint32_t buffer_data_bytes{0};
};

// GPU command buffer - batches and optimizes GPU commands
//
// Acceptance Criteria:
// 1. No per-node draw calls
//    - Batches multiple nodes into single draw calls
//    - Groups by material/pipeline state
// 2. Batched buffer updates
//    - Combines multiple buffer updates
//    - Minimizes update overhead
// 3. CPU↔GPU sync minimized
//    - Tracks GPU buffer state
//    - Only syncs when necessary
//    - Explicit sync points
class gpu_command_buffer {
  public:
    // Create command buffer with configuration and allocator
    explicit gpu_command_buffer(const gpu_command_buffer_config& config,
                               memory::allocator* alloc) noexcept
        : cfg(config), allocator(alloc) {
        if (allocator == nullptr) {
            return;
        }

        // Allocate command storage
        const size_t commands_size = sizeof(gpu_command) * cfg.max_commands;
        void* commands_mem = allocator->allocate(commands_size, alignof(gpu_command));
        if (commands_mem != nullptr) {
            commands = static_cast<gpu_command*>(commands_mem);
            // Initialize all commands
            for (uint32_t i = 0; i < cfg.max_commands; ++i) {
                commands[i] = gpu_command{};
            }
        }

        // Allocate buffer data storage (for buffer updates)
        void* buffer_data_mem =
            allocator->allocate(cfg.max_buffer_data_bytes, alignof(float));
        if (buffer_data_mem != nullptr) {
            buffer_data = static_cast<uint8_t*>(buffer_data_mem);
            // Zero-initialize buffer data
            for (uint32_t i = 0; i < cfg.max_buffer_data_bytes; ++i) {
                buffer_data[i] = 0;
            }
        }
    }

    ~gpu_command_buffer() noexcept {
        if (allocator != nullptr) {
            if (commands != nullptr) {
                allocator->deallocate(commands, sizeof(gpu_command) * cfg.max_commands);
            }
            if (buffer_data != nullptr) {
                allocator->deallocate(buffer_data, cfg.max_buffer_data_bytes);
            }
        }
    }

    // Disable copy and move
    gpu_command_buffer(const gpu_command_buffer&) = delete;
    gpu_command_buffer& operator=(const gpu_command_buffer&) = delete;
    gpu_command_buffer(gpu_command_buffer&&) = delete;
    gpu_command_buffer& operator=(gpu_command_buffer&&) = delete;

    // Translate scene diff to GPU commands
    // This is the main entry point for the GPU command backend
    [[nodiscard]] gpu_command_result translate_diff(const diff_change* changes,
                                                    uint32_t change_count,
                                                    const scene_graph* current_scene) noexcept;

    // Add a single GPU command
    [[nodiscard]] gpu_command_result add_command(const gpu_command& cmd) noexcept {
        if (commands == nullptr) {
            return gpu_command_result::out_of_memory;
        }

        if (command_count >= cfg.max_commands) {
            return gpu_command_result::command_limit_exceeded;
        }

        commands[command_count++] = cmd;

        // Update statistics
        update_stats(cmd);

        return gpu_command_result::success;
    }

    // Add buffer update data to internal storage
    // Returns offset in buffer_data where data was stored
    [[nodiscard]] gpu_command_result add_buffer_data(const void* data, uint32_t size,
                                                     uint32_t& out_offset) noexcept {
        if (buffer_data == nullptr || data == nullptr) {
            return gpu_command_result::out_of_memory;
        }

        if (buffer_data_offset + size > cfg.max_buffer_data_bytes) {
            return gpu_command_result::buffer_data_limit_exceeded;
        }

        // Copy data to internal storage
        const auto* src = static_cast<const uint8_t*>(data);
        for (uint32_t i = 0; i < size; ++i) {
            buffer_data[buffer_data_offset + i] = src[i];
        }

        out_offset = buffer_data_offset;
        buffer_data_offset += size;

        return gpu_command_result::success;
    }

    // Get command buffer for submission to GPU
    [[nodiscard]] const gpu_command* get_commands() const noexcept {
        return commands;
    }

    [[nodiscard]] uint32_t get_command_count() const noexcept {
        return command_count;
    }

    [[nodiscard]] const gpu_command_stats& get_stats() const noexcept {
        return stats;
    }

    // Clear command buffer (prepare for next frame)
    void clear() noexcept {
        command_count = 0;
        buffer_data_offset = 0;
        stats = gpu_command_stats{};
    }

    [[nodiscard]] bool is_valid() const noexcept {
        return commands != nullptr && buffer_data != nullptr;
    }

  private:
    // Update statistics based on command type
    void update_stats(const gpu_command& cmd) noexcept {
        stats.total_commands++;

        // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
        switch (cmd.type) {
        case gpu_command_type::update_buffer:
            stats.buffer_updates++;
            stats.buffer_data_bytes += cmd.buffer_update.size;
            break;
        case gpu_command_type::draw_indexed:
            stats.draw_calls++;
            break;
        case gpu_command_type::set_pipeline:
            stats.pipeline_changes++;
            break;
        case gpu_command_type::begin_batch:
            stats.batched_draws++;
            break;
        case gpu_command_type::sync_point:
            stats.sync_points++;
            break;
        default:
            break;
        }
        // NOLINTEND(cppcoreguidelines-pro-type-union-access)
    }

    gpu_command_buffer_config cfg;
    memory::allocator* allocator;

    // Command storage
    gpu_command* commands{nullptr};
    uint32_t command_count{0};

    // Buffer data storage (for buffer updates)
    uint8_t* buffer_data{nullptr};
    uint32_t buffer_data_offset{0};

    // Statistics
    gpu_command_stats stats{};
};

} // namespace aegis::core::frame
