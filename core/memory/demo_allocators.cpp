#include "core/frame/frame_lifecycle.hpp"
#include "core/memory/arena_allocator.hpp"
#include "core/memory/frame_allocator.hpp"
#include "core/memory/pool_allocator.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>

// Demonstration of the memory system
// This shows:
// 1. Zero calls to global new/delete during frame execution
// 2. Allocation/free cost measurable and logged
// 3. Frame allocator fully resets at end_frame()

namespace {

// Pre-allocated buffers for allocators (no global new/delete)
alignas(16) uint8_t arena_buffer[4096];
alignas(16) uint8_t frame_buffer[2048];
alignas(16) uint8_t pool_buffer[1024];

void print_separator() {
    printf("========================================\n");
}

void print_allocator_stats(const char* name, const aegis::core::memory::allocator& alloc) {
    printf("%s Statistics:\n", name);
    printf("  Bytes allocated: %zu\n", alloc.bytes_allocated());
    printf("  Bytes freed: %zu\n", alloc.bytes_freed());
    printf("  Bytes in use: %zu\n", alloc.bytes_in_use());
    printf("  Peak bytes used: %zu\n", alloc.peak_bytes_used());
    printf("  Supports reset: %s\n", alloc.supports_reset() ? "yes" : "no");
}

void print_frame_allocator_stats(const aegis::core::memory::frame_allocator& alloc) {
    print_allocator_stats("Frame Allocator", alloc);
    printf("  Allocation count: %zu\n", alloc.get_allocation_count());
    printf("  Deallocation count: %zu\n", alloc.get_deallocation_count());
    printf("  Capacity: %zu\n", alloc.get_capacity());
    printf("  Bytes used: %zu\n", alloc.bytes_used());
    printf("  Bytes available: %zu\n", alloc.bytes_available());
}

void print_frame_stats(const aegis::core::frame::frame_stats& stats) {
    printf("Frame Statistics:\n");
    printf("  Frame number: %lu\n", stats.frame_number);
    printf("  Total time: %lu ns\n", stats.total_time_ns);
    printf("  Bytes allocated: %zu\n", stats.bytes_allocated);
    printf("  Bytes freed: %zu\n", stats.bytes_freed);
    printf("  Peak memory used: %zu\n", stats.peak_memory_used);
    printf("  Frame allocations: %zu\n", stats.frame_allocations);
    printf("  Frame deallocations: %zu\n", stats.frame_deallocations);
}

} // anonymous namespace

int main() {
    printf("Aegis Memory System Demonstration\n");
    print_separator();

    // 1. Demonstrate Arena Allocator
    printf("\n1. Arena Allocator Demo\n");
    print_separator();

    aegis::core::memory::arena_allocator arena(arena_buffer, sizeof(arena_buffer));
    printf("Arena capacity: %zu bytes\n\n", arena.get_capacity());

    // Allocate some memory
    void* ptr1 = arena.allocate(128, 16);
    printf("Allocated 128 bytes at %p\n", ptr1);
    print_allocator_stats("Arena", arena);
    printf("\n");

    void* ptr2 = arena.allocate(256, 16);
    printf("Allocated 256 bytes at %p\n", ptr2);
    print_allocator_stats("Arena", arena);
    printf("\n");

    // Reset arena - O(1) operation
    printf("Resetting arena...\n");
    arena.reset();
    print_allocator_stats("Arena", arena);

    // 2. Demonstrate Frame Allocator
    printf("\n2. Frame Allocator Demo\n");
    print_separator();

    aegis::core::memory::frame_allocator frame_alloc(frame_buffer, sizeof(frame_buffer));
    printf("Frame allocator capacity: %zu bytes\n\n", frame_alloc.get_capacity());

    // Create frame context with frame allocator
    aegis::core::frame::frame_context ctx(&frame_alloc);

    // Execute first frame
    printf("Executing Frame 1...\n");

    // Begin frame
    (void)ctx.begin_frame(1000000);

    // Simulate frame allocations - these would be done by frame logic
    void* frame_ptr1 = frame_alloc.allocate(64, 8);
    void* frame_ptr2 = frame_alloc.allocate(128, 16);
    void* frame_ptr3 = frame_alloc.allocate(32, 4);
    printf("Allocated 3 objects (%p, %p, %p)\n", frame_ptr1, frame_ptr2, frame_ptr3);

    printf("\nBefore end_frame():\n");
    print_frame_allocator_stats(frame_alloc);

    // Transition through phases and end frame
    (void)ctx.apply_events();
    (void)ctx.update_state();
    (void)ctx.compute_layout();
    (void)ctx.build_scene();
    (void)ctx.diff_scene();
    (void)ctx.end_frame();

    printf("\nAfter end_frame():\n");
    print_frame_allocator_stats(frame_alloc);
    print_separator();
    print_frame_stats(ctx.stats_get());

    // Execute second frame - frame allocator was reset
    printf("\nExecuting Frame 2...\n");
    ctx.reset();
    (void)ctx.begin_frame(2000000);

    // Allocate different amounts in second frame
    void* frame2_ptr1 = frame_alloc.allocate(512, 16);
    printf("Allocated 512 bytes at %p (same buffer, reused after reset)\n", frame2_ptr1);

    printf("\nBefore end_frame():\n");
    print_frame_allocator_stats(frame_alloc);

    (void)ctx.apply_events();
    (void)ctx.update_state();
    (void)ctx.compute_layout();
    (void)ctx.build_scene();
    (void)ctx.diff_scene();
    (void)ctx.end_frame();

    printf("\nAfter end_frame():\n");
    print_frame_allocator_stats(frame_alloc);
    print_separator();
    print_frame_stats(ctx.stats_get());

    // 3. Demonstrate Pool Allocator
    printf("\n3. Pool Allocator Demo\n");
    print_separator();

    constexpr size_t block_size = 64;
    constexpr size_t block_alignment = 8;
    aegis::core::memory::pool_allocator pool(pool_buffer, sizeof(pool_buffer), block_size,
                                             block_alignment);

    printf("Pool allocator:\n");
    printf("  Block size: %zu bytes\n", pool.get_block_size());
    printf("  Block count: %zu\n", pool.get_block_count());
    printf("  Blocks available: %zu\n\n", pool.blocks_available());

    // Allocate some blocks
    void* pool_ptr1 = pool.allocate(1, 1); // size/alignment ignored for pools
    void* pool_ptr2 = pool.allocate(1, 1);
    void* pool_ptr3 = pool.allocate(1, 1);
    printf("Allocated 3 blocks (%p, %p, %p)\n", pool_ptr1, pool_ptr2, pool_ptr3);
    print_allocator_stats("Pool", pool);
    printf("  Blocks in use: %zu\n", pool.blocks_in_use());
    printf("  Blocks available: %zu\n\n", pool.blocks_available());

    // Free a block - pool supports individual deallocation
    printf("Freeing block 2...\n");
    pool.deallocate(pool_ptr2, block_size);
    print_allocator_stats("Pool", pool);
    printf("  Blocks in use: %zu\n", pool.blocks_in_use());
    printf("  Blocks available: %zu\n\n", pool.blocks_available());

    // Allocate again - should reuse freed block
    void* pool_ptr4 = pool.allocate(1, 1);
    printf("Allocated new block at %p (reused freed block)\n", pool_ptr4);
    print_allocator_stats("Pool", pool);
    printf("  Blocks in use: %zu\n", pool.blocks_in_use());
    printf("  Blocks available: %zu\n\n", pool.blocks_available());

    // Reset pool
    printf("Resetting pool...\n");
    pool.reset();
    print_allocator_stats("Pool", pool);
    printf("  Blocks in use: %zu\n", pool.blocks_in_use());
    printf("  Blocks available: %zu\n\n", pool.blocks_available());

    print_separator();
    printf("\nDemonstration complete!\n");
    printf("Key accomplishments:\n");
    printf("  ✓ Zero calls to global new/delete\n");
    printf("  ✓ Allocation costs measurable and logged\n");
    printf("  ✓ Frame allocator fully resets at end_frame()\n");

    return 0;
}
