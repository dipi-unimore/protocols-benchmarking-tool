#pragma once
#include "Receiver.hpp"
#include "ReassemblyBuffer.hpp"
#include <thread>

namespace pbt {

class UdpReceiver final : public Receiver {
public:
    UdpReceiver(const BenchmarkConfig& cfg,
                BoundedBlockingQueue<InboundPacket>& queue,
                int64_t ntp_offset_ns = 0);
    ~UdpReceiver() override;
    std::expected<void, Error> bind()  override;
    std::expected<void, Error> start() override;
    std::expected<void, Error> stop()  override;
    [[nodiscard]] std::size_t fragment_timeout_count() const noexcept {
        return reassembly_.timeout_count();
    }
private:
    void recv_loop(std::stop_token st);
    int              fd_{-1};
    std::jthread     thread_;
    ReassemblyBuffer reassembly_;
};

}  // namespace pbt
