#pragma once

namespace pbt {

struct LatencyStats {
    double mean_us{0.0};
    double stddev_us{0.0};
    double min_us{0.0};
    double max_us{0.0};
    double p50_us{0.0};
    double p95_us{0.0};
    double p99_us{0.0};
    double p999_us{0.0};
    double jitter_us{0.0};
};

}  // namespace pbt
