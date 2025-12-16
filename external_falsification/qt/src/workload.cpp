#include "workload.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace aegis::benchmark {

QtWorkload::QtWorkload() = default;

QtWorkload::~QtWorkload() {
    reset();
}

void QtWorkload::reset() {
    // Free all allocations
    for (void* ptr : allocations_) {
        std::free(ptr);
    }
    allocations_.clear();
    operations_.clear();
}

void QtWorkload::perform_allocations() {
    for (size_t i = 0; i < Config::allocations_per_frame; ++i) {
        const size_t size_idx = i % Config::allocation_size_count;
        const size_t size = Config::allocation_sizes[size_idx];
        
        void* ptr = std::malloc(size);
        if (ptr) {
            // Touch the memory to ensure it's actually allocated
            std::memset(ptr, static_cast<int>(i), size);
            allocations_.push_back(ptr);
        }
    }
}

void QtWorkload::perform_operations() {
    for (size_t i = 0; i < Config::operations_per_frame; ++i) {
        const uint64_t result = i * i + i;
        operations_.push_back(result);
    }
}

void QtWorkload::execute_frame() {
    reset();
    perform_allocations();
    perform_operations();
}

PercentileMetrics calculate_percentiles(const std::vector<uint64_t>& timings) {
    if (timings.empty()) {
        return PercentileMetrics{};
    }

    std::vector<uint64_t> sorted = timings;
    std::sort(sorted.begin(), sorted.end());

    const size_t n = sorted.size();
    
    auto get_percentile = [&](double p) -> uint64_t {
        const size_t idx = static_cast<size_t>(std::floor((n * p) / 100.0));
        return sorted[std::min(idx, n - 1)];
    };

    // Calculate mean
    uint64_t sum = 0;
    for (uint64_t t : sorted) {
        sum += t;
    }
    const uint64_t mean = sum / n;

    // Calculate variance
    double variance_sum = 0.0;
    for (uint64_t t : sorted) {
        const double diff = static_cast<double>(t) - static_cast<double>(mean);
        variance_sum += diff * diff;
    }
    const double variance = variance_sum / static_cast<double>(n);

    return PercentileMetrics{
        .min_ns = sorted[0],
        .p50_ns = get_percentile(50),
        .p90_ns = get_percentile(90),
        .p95_ns = get_percentile(95),
        .p99_ns = get_percentile(99),
        .p99_9_ns = get_percentile(99.9),
        .max_ns = sorted[n - 1],
        .mean_ns = mean,
        .variance = variance
    };
}

} // namespace aegis::benchmark
