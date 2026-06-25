#pragma once
#include "Sender.hpp"
#include <zmq.hpp>

namespace pbt {

class ZmqTcpSender final : public Sender {
public:
    ZmqTcpSender(const BenchmarkConfig& cfg, std::unique_ptr<PayloadSource> src,
                 std::unique_ptr<Serializer> ser, std::unique_ptr<Compressor> cmp,
                 int64_t ntp_offset_ns = 0);
    std::expected<void, Error> connect()    override;
    std::expected<void, Error> disconnect() override;
protected:
    std::expected<void, Error> do_send(std::span<const uint8_t> wire) override;
private:
    zmq::context_t ctx_{1};
    zmq::socket_t  socket_;
};

}  // namespace pbt
