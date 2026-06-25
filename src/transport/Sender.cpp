#include "pbt/transport/Sender.hpp"
#include "pbt/transport/TcpSender.hpp"
#include "pbt/transport/UdpSender.hpp"
#include "pbt/transport/ZmqTcpSender.hpp"
#include "pbt/transport/MqttTcpSender.hpp"
#include "pbt/payload/PayloadSource.hpp"
#include "pbt/serialization/Serializer.hpp"
#include "pbt/compression/Compressor.hpp"
#include <cstring>

namespace pbt {

Sender::Sender(const BenchmarkConfig& cfg,
               std::unique_ptr<PayloadSource> src,
               std::unique_ptr<Serializer>    ser,
               std::unique_ptr<Compressor>    cmp,
               int64_t ntp_offset_ns)
    : config_(cfg)
    , source_(std::move(src))
    , serializer_(std::move(ser))
    , compressor_(std::move(cmp))
    , ntp_offset_ns_(ntp_offset_ns) {}

Bytes Sender::assemble_wire(const WireHeader& hdr, std::span<const uint8_t> payload) {
    Bytes wire(sizeof(WireHeader) + payload.size());
    std::memcpy(wire.data(), &hdr, sizeof(WireHeader));
    std::memcpy(wire.data() + sizeof(WireHeader), payload.data(), payload.size());
    return wire;
}

std::expected<WireHeader, Error> Sender::send(uint64_t seq_id, bool is_warmup) {
    auto raw = source_->next();

    WireHeader hdr;
    hdr.sequence_id   = seq_id;
    hdr.ntp_offset_ns = ntp_offset_ns_;
    hdr.serializer_id = static_cast<uint8_t>(config_.serializer);
    hdr.compressor_id = static_cast<uint8_t>(config_.compression);
    hdr.qos           = static_cast<uint8_t>(config_.mqtt_qos);
    hdr.flags         = is_warmup ? 0x04u : 0x00u;

    hdr.ts_created_ns = now_ns();
    auto ser_result = serializer_->serialize(raw);
    if (!ser_result) return std::unexpected(ser_result.error());
    hdr.ts_serialized_ns = now_ns();

    auto cmp_result = compressor_->compress(*ser_result);
    if (!cmp_result) return std::unexpected(cmp_result.error());
    hdr.ts_compressed_ns = now_ns();

    hdr.payload_size  = static_cast<uint32_t>(cmp_result->size());
    hdr.original_size = static_cast<uint32_t>(ser_result->size());
    hdr.payload_crc32 = crc32(cmp_result->data(), cmp_result->size());

    // Stamp ts_sent BEFORE assemble+do_send so value is in transmitted header
    hdr.ts_sent_ns = now_ns();
    auto wire = assemble_wire(hdr, *cmp_result);
    auto res  = do_send(wire);
    if (!res) return std::unexpected(res.error());
    return hdr;
}

std::expected<void, Error> Sender::send_sentinel(uint64_t last_seq_id) {
    WireHeader hdr;
    hdr.sequence_id   = last_seq_id;
    hdr.ntp_offset_ns = ntp_offset_ns_;
    hdr.serializer_id = static_cast<uint8_t>(config_.serializer);
    hdr.compressor_id = static_cast<uint8_t>(config_.compression);
    hdr.set_sentinel();
    hdr.ts_sent_ns    = now_ns();
    hdr.payload_size  = 0;
    hdr.original_size = 0;
    hdr.payload_crc32 = 0;
    auto wire = assemble_wire(hdr, {});
    return do_send(wire);
}

std::unique_ptr<Sender> Sender::create(const BenchmarkConfig& cfg,
                                        std::unique_ptr<PayloadSource> src,
                                        std::unique_ptr<Serializer>    ser,
                                        std::unique_ptr<Compressor>    cmp,
                                        int64_t ntp_offset_ns) {
    switch (cfg.protocol) {
        case Protocol::MqttTcp:
            return std::make_unique<MqttTcpSender>(cfg, std::move(src),
                                                    std::move(ser), std::move(cmp),
                                                    ntp_offset_ns);
        case Protocol::ZmqTcp:
            return std::make_unique<ZmqTcpSender>(cfg, std::move(src),
                                                   std::move(ser), std::move(cmp),
                                                   ntp_offset_ns);
        case Protocol::Tcp:
            return std::make_unique<TcpSender>(cfg, std::move(src),
                                               std::move(ser), std::move(cmp),
                                               ntp_offset_ns);
        case Protocol::Udp:
            return std::make_unique<UdpSender>(cfg, std::move(src),
                                               std::move(ser), std::move(cmp),
                                               ntp_offset_ns);
    }
    return std::make_unique<TcpSender>(cfg, std::move(src),
                                       std::move(ser), std::move(cmp),
                                       ntp_offset_ns);
}

}  // namespace pbt
