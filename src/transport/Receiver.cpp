#include "pbt/transport/Receiver.hpp"
#include "pbt/transport/TcpReceiver.hpp"
#include "pbt/transport/UdpReceiver.hpp"
#include "pbt/transport/ZmqTcpReceiver.hpp"
#include "pbt/transport/MqttTcpReceiver.hpp"
#include <cstring>

namespace pbt {

Receiver::Receiver(const BenchmarkConfig& cfg,
                   BoundedBlockingQueue<InboundPacket>& queue,
                   NtpInfo ntp_info)
    : config_(cfg), queue_(queue), ntp_info_(ntp_info) {}

bool Receiver::parse_wire(std::span<const uint8_t> wire,
                           WireHeader& hdr_out,
                           std::span<const uint8_t>& payload_out) {
    if (wire.size() < sizeof(WireHeader)) return false;
    std::memcpy(&hdr_out, wire.data(), sizeof(WireHeader));
    if (hdr_out.magic != 0x50425401u) return false;
    if (wire.size() < sizeof(WireHeader) + hdr_out.payload_size) return false;
    payload_out = wire.subspan(sizeof(WireHeader), hdr_out.payload_size);
    return true;
}

void Receiver::on_wire_bytes(std::span<const uint8_t> wire) {
    TimePoint ts = Clock::now();
    WireHeader hdr;
    std::span<const uint8_t> payload;
    if (!parse_wire(wire, hdr, payload)) return;

    if (hdr.is_sentinel()) {
        sentinel_received_      = true;
        msgs_sent_from_sentinel_ = hdr.sequence_id + 1;
        return;
    }

    InboundPacket pkt;
    pkt.header      = hdr;
    pkt.wire_payload = Bytes(payload.begin(), payload.end());
    pkt.ts_received  = ts;
    pkt.receiver_ntp_offset_ns      = ntp_info_.offset_ns;
    pkt.receiver_ntp_uncertainty_ns = ntp_info_.uncertainty_ns;

    if (!queue_.try_push(std::move(pkt)))
        overflow_count_.fetch_add(1, std::memory_order_relaxed);
}

std::unique_ptr<Receiver> Receiver::create(const BenchmarkConfig& cfg,
                                             BoundedBlockingQueue<InboundPacket>& queue,
                                             NtpInfo ntp_info) {
    switch (cfg.protocol) {
        case Protocol::MqttTcp:
            return std::make_unique<MqttTcpReceiver>(cfg, queue, ntp_info);
        case Protocol::ZmqTcp:
            return std::make_unique<ZmqTcpReceiver>(cfg, queue, ntp_info);
        case Protocol::Tcp:
            return std::make_unique<TcpReceiver>(cfg, queue, ntp_info);
        case Protocol::Udp:
            return std::make_unique<UdpReceiver>(cfg, queue, ntp_info);
    }
    return std::make_unique<TcpReceiver>(cfg, queue, ntp_info);
}

}  // namespace pbt
