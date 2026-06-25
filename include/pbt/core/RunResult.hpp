#pragma once
#include "BenchmarkConfig.hpp"
#include "LatencyStats.hpp"
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>

namespace pbt {

struct RunResult {
    BenchmarkConfig config;

    std::optional<uint64_t> msgs_sent;   // nullopt if sentinel lost (UDP)
    uint64_t msgs_received{0};
    uint64_t out_of_order_count{0};
    uint64_t duplicate_count{0};
    uint64_t overflow_count{0};
    uint64_t fragment_timeout_count{0};
    uint64_t crc_error_count{0};
    uint64_t total_seq_gap{0};

    double packet_loss_pct{0.0};
    double throughput_msgs_per_sec{0.0};
    double throughput_bytes_per_sec{0.0};

    LatencyStats e2e;
    LatencyStats serialization;
    LatencyStats compression;
    LatencyStats transport;
    LatencyStats decompression;
    LatencyStats deserialization;

    [[nodiscard]] nlohmann::json to_json() const;
};

}  // namespace pbt
