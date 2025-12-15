#pragma once

#include "core/frame/scene_graph.hpp"
#include "core/memory/allocator.hpp"

#include <cstddef>
#include <cstdint>

namespace aegis::core::frame {

// GPU command types - minimal set for efficient rendering
enum class gpu_command_type : uint8_t {
    noop = 0,          // No operation (padding)
    update_buffer = 1, // Update vertex/uniform buffer
    draw_indexed = 2,  // Indexed draw call
    set_pipeline = 3,  // Set graphics pipeline state
    begin_batch = 4,   // Begin batched draw sequence
    end_batch = 5,     // End batched draw sequence
    sync_point = 6     // CPU↔GPU synchronization point
};

// GPU buffer update descriptor
// Used to batch multiple buffer updates together
struct gpu_buffer_update {
    uint32_t buffer_id{0};     // Buffer identifier
    uint32_t offset{0};        // Offset in buffer (bytes)
    uint32_t size{0};          // Size of update (bytes)
    const void* data{nullptr}; // Pointer to update data (owned by command buffer)
};

// GPU draw call descriptor
// Represents a single indexed draw call
struct gpu_draw_indexed {
    uint32_t index_count{0};    // Number of indices
    uint32_t instance_count{1}; // Number of instances
    uint32_t first_index{0};    // First index in index buffer
    int32_t vertex_offset{0};   // Offset added to vertex index
    uint32_t first_instance{0}; // First instance ID
};

// GPU pipeline state descriptor
// Represents graphics pipeline configuration
struct gpu_pipeline_state {
    uint32_t pipeline_id{0};       // Pipeline identifier
    uint32_t vertex_buffer_id{0};  // Vertex buffer binding
    uint32_t index_buffer_id{0};   // Index buffer binding
    uint32_t uniform_buffer_id{0}; // Uniform buffer binding
};

// GPU command - single atomic GPU operation
// Commands are POD types for cache-friendly processing
// NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
struct gpu_command {
    gpu_command_type type{gpu_command_type::noop};
    uint8_t padding[3]{0, 0, 0}; // Align to 4 bytes

    // Command payload (union for memory efficiency)
    // Union is used intentionally for cache-friendly POD layout
    union {
        gpu_buffer_update buffer_update;
        gpu_draw_indexed draw_indexed;
        gpu_pipeline_state pipeline_state;
        uint32_t batch_size; // For begin_batch/end_batch
    };

    // Default constructor
    gpu_command() noexcept : buffer_update{} {
    }

    // Create buffer update command
    static gpu_command create_buffer_update(uint32_t buffer_id, uint32_t offset, uint32_t size,
                                            const void* data) noexcept {
        gpu_command cmd;
        cmd.type = gpu_command_type::update_buffer;
        cmd.buffer_update = {buffer_id, offset, size, data};
        return cmd;
    }

    // Create draw command
    static gpu_command create_draw_indexed(uint32_t index_count, uint32_t instance_count = 1,
                                           uint32_t first_index = 0, int32_t vertex_offset = 0,
                                           uint32_t first_instance = 0) noexcept {
        gpu_command cmd;
        cmd.type = gpu_command_type::draw_indexed;
        cmd.draw_indexed = {index_count, instance_count, first_index, vertex_offset,
                            first_instance};
        return cmd;
    }

    // Create pipeline state command
    static gpu_command create_set_pipeline(uint32_t pipeline_id, uint32_t vertex_buffer_id = 0,
                                           uint32_t index_buffer_id = 0,
                                           uint32_t uniform_buffer_id = 0) noexcept {
        gpu_command cmd;
        cmd.type = gpu_command_type::set_pipeline;
        cmd.pipeline_state = {pipeline_id, vertex_buffer_id, index_buffer_id, uniform_buffer_id};
        return cmd;
    }

    // Create batch begin command
    static gpu_command create_begin_batch(uint32_t batch_size) noexcept {
        gpu_command cmd;
        cmd.type = gpu_command_type::begin_batch;
        cmd.batch_size = batch_size;
        return cmd;
    }

    // Create batch end command
    static gpu_command create_end_batch() noexcept {
        gpu_command cmd;
        cmd.type = gpu_command_type::end_batch;
        cmd.batch_size = 0;
        return cmd;
    }

    // Create sync point command
    static gpu_command create_sync_point() noexcept {
        gpu_command cmd;
        cmd.type = gpu_command_type::sync_point;
        return cmd;
    }
};

// Ensure gpu_command is POD and cache-friendly
static_assert(sizeof(gpu_command) <= 32, "gpu_command should fit in cache line");
static_assert(alignof(gpu_command) <= 8, "gpu_command should have reasonable alignment");
// NOLINTEND(cppcoreguidelines-pro-type-union-access)

} // namespace aegis::core::frame
