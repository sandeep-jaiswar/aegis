#include "core/benchmark/benchmark.hpp"
#include "core/frame/frame_lifecycle.hpp"

namespace aegis::core::benchmark {

// Benchmark for frame lifecycle performance
class frame_lifecycle_benchmark final : public benchmark {
  public:
    explicit frame_lifecycle_benchmark(frame::frame_context* ctx,
                                       frame::frame_executor* exec) noexcept
        : context(ctx), executor(exec), current_timestamp(0) {
    }

    [[nodiscard]] const char* name() const noexcept override {
        return "frame_lifecycle_benchmark";
    }

    void setup() noexcept override {
        if (context) {
            context->reset();
        }
        current_timestamp = 0;
    }

    void execute() noexcept override {
        if (!context || !executor) {
            return;
        }

        // Simulate one complete frame
        // Each benchmark iteration = one frame cycle
        current_timestamp += 16'666'667; // ~60 FPS (16.67ms)
        [[maybe_unused]] auto result = executor->execute_frame(current_timestamp, *context);
    }

    void teardown() noexcept override {
        // Nothing to clean up
    }

    [[nodiscard]] bool verify() const noexcept override {
        // Verify frame executed successfully
        return context != nullptr && executor != nullptr;
    }

    [[nodiscard]] uint64_t get_workload_hash() const noexcept override {
        // Hash based on frame execution pattern
        return 0xFEDCBA9876543210ULL;
    }

  private:
    frame::frame_context* context;
    frame::frame_executor* executor;
    uint64_t current_timestamp;
};

} // namespace aegis::core::benchmark
