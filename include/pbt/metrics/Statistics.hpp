#pragma once
#include "pbt/core/LatencyStats.hpp"
#include <span>
#include <vector>

namespace pbt {

namespace stats {

// p ∈ [0,1]; input must be sorted
[[nodiscard]] double percentile(std::span<const double> sorted, double p) noexcept;

// Mean |d[i] - d[i-1]|
[[nodiscard]] double jitter(std::span<const double> samples) noexcept;

// Compute full LatencyStats from unsorted samples (sorts in-place)
[[nodiscard]] LatencyStats compute(std::vector<double>& samples_us);

}  // namespace stats

}  // namespace pbt
