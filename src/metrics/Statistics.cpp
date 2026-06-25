#include "pbt/metrics/Statistics.hpp"
#include "pbt/metrics/WelfordAccumulator.hpp"
#include <algorithm>
#include <cmath>
#include <ranges>

namespace pbt::stats {

double percentile(std::span<const double> sorted, double p) noexcept {
    if (sorted.empty()) return 0.0;
    double idx = p * static_cast<double>(sorted.size() - 1);
    auto   lo  = static_cast<std::size_t>(idx);
    auto   hi  = lo + 1;
    if (hi >= sorted.size()) return sorted.back();
    double frac = idx - static_cast<double>(lo);
    return sorted[lo] * (1.0 - frac) + sorted[hi] * frac;
}

double jitter(std::span<const double> samples) noexcept {
    if (samples.size() < 2) return 0.0;
    double sum = 0.0;
    for (std::size_t i = 1; i < samples.size(); ++i)
        sum += std::abs(samples[i] - samples[i - 1]);
    return sum / static_cast<double>(samples.size() - 1);
}

LatencyStats compute(std::vector<double>& samples_us) {
    if (samples_us.empty()) return {};
    std::ranges::sort(samples_us);
    WelfordAccumulator acc;
    for (double v : samples_us) acc.update(v);
    LatencyStats s;
    s.mean_us   = acc.mean();
    s.stddev_us = acc.stddev();
    s.min_us    = acc.min();
    s.max_us    = acc.max();
    s.p50_us    = percentile(samples_us, 0.50);
    s.p95_us    = percentile(samples_us, 0.95);
    s.p99_us    = percentile(samples_us, 0.99);
    s.p999_us   = percentile(samples_us, 0.999);
    s.jitter_us = jitter(samples_us);
    return s;
}

}  // namespace pbt::stats
