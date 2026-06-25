#pragma once
#include "Receiver.hpp"
#include <thread>
#include <zmq.hpp>

namespace pbt {

class ZmqTcpReceiver final : public Receiver {
public:
    ZmqTcpReceiver(const BenchmarkConfig& cfg,
                   BoundedBlockingQueue<InboundPacket>& queue,
                   int64_t ntp_offset_ns = 0);
    std::expected<void, Error> bind()  override;
    std::expected<void, Error> start() override;
    std::expected<void, Error> stop()  override;
private:
    void recv_loop(std::stop_token st);
    zmq::context_t ctx_{1};
    zmq::socket_t  socket_;
    std::jthread   thread_;
};

}  // namespace pbt
