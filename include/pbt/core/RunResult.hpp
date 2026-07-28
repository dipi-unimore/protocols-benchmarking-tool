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

    // NTP sync diagnostics (constant for the run — see PacketProcessor first-packet capture)
    std::string ntp_server;                    // server actually used (set by main.cpp), empty if disabled
    bool        ntp_disabled{true};
    int64_t     ntp_sender_offset_ns{0};
    int64_t     ntp_receiver_offset_ns{0};
    int64_t     ntp_sender_uncertainty_ns{0};
    int64_t     ntp_receiver_uncertainty_ns{0};
    double      ntp_sync_uncertainty_us{0.0};  // ± band on e2e_delay_us from clock-sync error

    [[nodiscard]] nlohmann::json to_json() const;
};

}  // namespace pbt
