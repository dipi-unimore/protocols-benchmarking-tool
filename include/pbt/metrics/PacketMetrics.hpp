#pragma once
#include "pbt/core/Types.hpp"
#include <cstdint>

namespace pbt {

// NTP offset convention: true_time = local_reading + offset (RFC 4330).
// So true_send = ts_sent_ns + sender_offset_ns, true_recv = ts_received_ns + receiver_offset_ns,
// and e2e = true_recv - true_send = (ts_received_ns - ts_sent_ns) + (receiver_offset_ns - sender_offset_ns).
//
// ntp_disabled comes from WireHeader::is_ntp_disabled(), set by the sender whenever it skipped
// NTP sync (--no-ntp, or an auto-detected loopback target — see main.cpp). It forces both offsets
// to 0 regardless of what either side's local NtpInfo holds, so a receiver that independently ran
// its own (real, nonzero) NTP query still can't reintroduce cross-query sync noise into a run the
// sender flagged as not needing correction (e.g. same-host clock, no true offset to correct for).
[[nodiscard]] inline double compute_e2e_delay_us(int64_t ts_received_ns, int64_t ts_sent_ns,
                                                  int64_t sender_offset_ns,
                                                  int64_t receiver_offset_ns,
                                                  bool ntp_disabled) noexcept {
    const int64_t send_off = ntp_disabled ? 0 : sender_offset_ns;
    const int64_t recv_off = ntp_disabled ? 0 : receiver_offset_ns;
    return ns_to_us((ts_received_ns - ts_sent_ns) + (recv_off - send_off));
}

struct PacketMetrics {
    uint64_t seq_id{0};
    uint32_t payload_size_bytes{0};
    uint32_t wire_size_bytes{0};

    int64_t ts_created_ns{0};
    int64_t ts_serialized_ns{0};
    int64_t ts_compressed_ns{0};
    int64_t ts_sent_ns{0};
    int64_t ts_received_ns{0};
    int64_t ts_decompressed_ns{0};
    int64_t ts_deserialized_ns{0};
    int64_t ts_processed_ns{0};
    int64_t ntp_offset_sender_ns{0};
    int64_t ntp_offset_receiver_ns{0};
    int64_t ntp_uncertainty_sender_ns{0};
    int64_t ntp_uncertainty_receiver_ns{0};
    bool    ntp_disabled{true};

    double e2e_delay_us{0.0};
    double ntp_sync_uncertainty_us{0.0};  // ± band on e2e_delay_us from clock-sync error; 0 when ntp_disabled
    double serialization_us{0.0};
    double compression_us{0.0};
    double transport_us{0.0};
    double decompression_us{0.0};
    double deserialization_us{0.0};
    double processing_us{0.0};
    double jitter_us{0.0};

    bool    is_warmup{false};
    bool    crc_ok{true};
    bool    is_out_of_order{false};
    bool    is_duplicate{false};
    int64_t seq_gap{0};
};

}  // namespace pbt
