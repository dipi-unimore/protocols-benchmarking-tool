#include "pbt/core/RunResult.hpp"
#include <cmath>

namespace pbt {

static nlohmann::json latency_to_json(const LatencyStats& s) {
    return {
        {"mean_us",   s.mean_us},
        {"stddev_us", s.stddev_us},
        {"min_us",    s.min_us},
        {"max_us",    s.max_us},
        {"p50_us",    s.p50_us},
        {"p95_us",    s.p95_us},
        {"p99_us",    s.p99_us},
        {"p999_us",   s.p999_us},
        {"jitter_us", s.jitter_us},
    };
}

nlohmann::json RunResult::to_json() const {
    nlohmann::json j;
    j["config"] = config.to_json();
    if (msgs_sent.has_value())
        j["msgs_sent"] = *msgs_sent;
    else
        j["msgs_sent"] = nullptr;
    j["msgs_received"]           = msgs_received;
    j["out_of_order_count"]      = out_of_order_count;
    j["duplicate_count"]         = duplicate_count;
    j["overflow_count"]          = overflow_count;
    j["fragment_timeout_count"]  = fragment_timeout_count;
    j["crc_error_count"]         = crc_error_count;
    j["total_seq_gap"]           = total_seq_gap;
    if (msgs_sent.has_value())
        j["packet_loss_pct"]     = packet_loss_pct;
    else
        j["packet_loss_pct"]     = nullptr;
    j["throughput_msgs_per_sec"] = throughput_msgs_per_sec;
    j["throughput_bytes_per_sec"]= throughput_bytes_per_sec;
    j["latency"] = {
        {"e2e",             latency_to_json(e2e)},
        {"serialization",   latency_to_json(serialization)},
        {"compression",     latency_to_json(compression)},
        {"transport",       latency_to_json(transport)},
        {"decompression",   latency_to_json(decompression)},
        {"deserialization", latency_to_json(deserialization)},
    };
    j["ntp"] = {
        {"server",                  ntp_server},
        {"disabled",                ntp_disabled},
        {"sender_offset_ns",        ntp_sender_offset_ns},
        {"receiver_offset_ns",      ntp_receiver_offset_ns},
        {"sender_uncertainty_ns",   ntp_sender_uncertainty_ns},
        {"receiver_uncertainty_ns", ntp_receiver_uncertainty_ns},
        {"sync_uncertainty_us",     ntp_sync_uncertainty_us},
    };
    return j;
}

}  // namespace pbt
