#include "pbt/transport/ZmqTcpReceiver.hpp"
#include <format>

namespace pbt {

ZmqTcpReceiver::ZmqTcpReceiver(const BenchmarkConfig& cfg,
                                 BoundedBlockingQueue<InboundPacket>& queue,
                                 NtpInfo ntp_info)
    : Receiver(cfg, queue, ntp_info)
    , socket_(ctx_, cfg.zmq_pattern == ZmqPattern::PubSub ? zmq::socket_type::sub
                                                           : zmq::socket_type::pull) {}

std::expected<void, Error> ZmqTcpReceiver::bind() {
    try {
        if (config_.zmq_pattern == ZmqPattern::PubSub) {
            // SUB connects to PUB's bind address
            auto endpoint = std::format("tcp://{}:{}", config_.host, config_.port);
            socket_.connect(endpoint);
            socket_.set(zmq::sockopt::subscribe, config_.zmq_topic);
        } else {
            // PULL binds; PUSH connects
            auto endpoint = std::format("tcp://0.0.0.0:{}", config_.port);
            socket_.bind(endpoint);
        }
        return {};
    } catch (const zmq::error_t& e) {
        return std::unexpected(Error{e.what()});
    }
}

std::expected<void, Error> ZmqTcpReceiver::start() {
    thread_ = std::jthread([this](std::stop_token st) { recv_loop(st); });
    return {};
}

std::expected<void, Error> ZmqTcpReceiver::stop() {
    thread_.request_stop();
    socket_.close();
    return {};
}

void ZmqTcpReceiver::recv_loop(std::stop_token st) {
    while (!st.stop_requested()) {
        zmq::message_t msg;
        auto result = socket_.recv(msg, zmq::recv_flags::dontwait);
        if (!result) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            continue;
        }
        // For PUB/SUB: first frame is topic, second is wire data
        if (config_.zmq_pattern == ZmqPattern::PubSub) {
            if (socket_.get(zmq::sockopt::rcvmore)) {
                zmq::message_t wire_msg;
                if (socket_.recv(wire_msg, zmq::recv_flags::none)) {
                    on_wire_bytes(std::span<const uint8_t>(
                        static_cast<const uint8_t*>(wire_msg.data()), wire_msg.size()));
                }
            }
        } else {
            on_wire_bytes(std::span<const uint8_t>(
                static_cast<const uint8_t*>(msg.data()), msg.size()));
        }
    }
}

}  // namespace pbt
