#include "pbt/transport/MqttTcpReceiver.hpp"
#include <format>

namespace pbt {

MqttTcpReceiver::MqttTcpReceiver(const BenchmarkConfig& cfg,
                                   BoundedBlockingQueue<InboundPacket>& queue,
                                   NtpInfo ntp_info)
    : Receiver(cfg, queue, ntp_info)
    , client_(std::format("tcp://{}:{}", cfg.host, cfg.port), "pbt-receiver") {}

std::expected<void, Error> MqttTcpReceiver::bind() {
    client_.set_callback(*this);
    return {};
}

std::expected<void, Error> MqttTcpReceiver::start() {
    try {
        mqtt::connect_options opts;
        opts.set_clean_session(true);
        client_.connect(opts)->wait();
        client_.subscribe(config_.mqtt_topic,
                           static_cast<int>(config_.mqtt_qos))->wait();
        return {};
    } catch (const mqtt::exception& e) {
        return std::unexpected(Error{e.what()});
    }
}

std::expected<void, Error> MqttTcpReceiver::stop() {
    try {
        if (client_.is_connected()) client_.disconnect()->wait();
        return {};
    } catch (const mqtt::exception& e) {
        return std::unexpected(Error{e.what()});
    }
}

void MqttTcpReceiver::message_arrived(mqtt::const_message_ptr msg) {
    const auto& payload = msg->get_payload();
    on_wire_bytes(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(payload.data()), payload.size()));
}

}  // namespace pbt
