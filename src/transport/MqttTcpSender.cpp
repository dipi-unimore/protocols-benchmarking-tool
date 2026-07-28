#include "pbt/transport/MqttTcpSender.hpp"
#include "pbt/payload/PayloadSource.hpp"
#include "pbt/serialization/Serializer.hpp"
#include "pbt/compression/Compressor.hpp"
#include <format>

namespace pbt {

MqttTcpSender::MqttTcpSender(const BenchmarkConfig& cfg,
                               std::unique_ptr<PayloadSource> src,
                               std::unique_ptr<Serializer>    ser,
                               std::unique_ptr<Compressor>    cmp,
                               NtpInfo ntp_info)
    : Sender(cfg, std::move(src), std::move(ser), std::move(cmp), ntp_info)
    , client_(std::format("tcp://{}:{}", cfg.host, cfg.port), "pbt-sender") {}

std::expected<void, Error> MqttTcpSender::connect() {
    try {
        mqtt::connect_options opts;
        opts.set_clean_session(true);
        client_.connect(opts)->wait();
        return {};
    } catch (const mqtt::exception& e) {
        return std::unexpected(Error{e.what()});
    }
}

std::expected<void, Error> MqttTcpSender::disconnect() {
    try {
        if (client_.is_connected()) client_.disconnect()->wait();
        return {};
    } catch (const mqtt::exception& e) {
        return std::unexpected(Error{e.what()});
    }
}

std::expected<void, Error> MqttTcpSender::do_send(std::span<const uint8_t> wire) {
    try {
        auto msg = mqtt::make_message(
            config_.mqtt_topic,
            std::string(reinterpret_cast<const char*>(wire.data()), wire.size()),
            static_cast<int>(config_.mqtt_qos), false);
        client_.publish(msg)->wait();
        return {};
    } catch (const mqtt::exception& e) {
        return std::unexpected(Error{e.what()});
    }
}

}  // namespace pbt
