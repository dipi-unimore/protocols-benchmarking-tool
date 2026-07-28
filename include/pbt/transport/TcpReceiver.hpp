#pragma once
#include "Receiver.hpp"
#include <thread>

namespace pbt {

class TcpReceiver final : public Receiver {
public:
    TcpReceiver(const BenchmarkConfig& cfg,
                BoundedBlockingQueue<InboundPacket>& queue,
                NtpInfo ntp_info = {});
    ~TcpReceiver() override;
    std::expected<void, Error> bind()  override;
    std::expected<void, Error> start() override;
    std::expected<void, Error> stop()  override;
private:
    void accept_loop(std::stop_token st);
    void client_loop(int client_fd, std::stop_token st);
    int          server_fd_{-1};
    std::jthread accept_thread_;
};

}  // namespace pbt
