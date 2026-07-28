#include "pbt/transport/ZmqTcpSender.hpp"
#include "pbt/payload/PayloadSource.hpp"
#include "pbt/serialization/Serializer.hpp"
#include "pbt/compression/Compressor.hpp"
#include <format>

namespace pbt {

ZmqTcpSender::ZmqTcpSender(const BenchmarkConfig& cfg,
                             std::unique_ptr<PayloadSource> src,
                             std::unique_ptr<Serializer>    ser,
                             std::unique_ptr<Compressor>    cmp,
                             NtpInfo ntp_info)
    : Sender(cfg, std::move(src), std::move(ser), std::move(cmp), ntp_info)
    , socket_(ctx_, cfg.zmq_pattern == ZmqPattern::PubSub ? zmq::socket_type::pub
                                                           : zmq::socket_type::push) {}

std::expected<void, Error> ZmqTcpSender::connect() {
    try {
        if (config_.zmq_pattern == ZmqPattern::PubSub) {
            // PUB binds; SUB connects to it
            auto endpoint = std::format("tcp://0.0.0.0:{}", config_.port);
            socket_.bind(endpoint);
        } else {
            // PUSH connects to PULL (receiver binds)
            auto endpoint = std::format("tcp://{}:{}", config_.host, config_.port);
            socket_.connect(endpoint);
        }
        return {};
    } catch (const zmq::error_t& e) {
        return std::unexpected(Error{e.what()});
    }
}

std::expected<void, Error> ZmqTcpSender::disconnect() {
    socket_.close();
    return {};
}

std::expected<void, Error> ZmqTcpSender::do_send(std::span<const uint8_t> wire) {
    try {
        if (config_.zmq_pattern == ZmqPattern::PubSub && !config_.zmq_topic.empty()) {
            // Multipart: [topic][wire]
            zmq::message_t topic_msg(config_.zmq_topic.data(),
                                      config_.zmq_topic.size());
            socket_.send(topic_msg, zmq::send_flags::sndmore);
        }
        zmq::message_t msg(wire.data(), wire.size());
        socket_.send(msg, zmq::send_flags::none);
        return {};
    } catch (const zmq::error_t& e) {
        return std::unexpected(Error{e.what()});
    }
}

}  // namespace pbt
