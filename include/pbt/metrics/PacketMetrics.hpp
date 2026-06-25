#pragma once
#include <cstdint>

namespace pbt {

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

    double e2e_delay_us{0.0};
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
