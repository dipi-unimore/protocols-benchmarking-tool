#pragma once
#include "Receiver.hpp"
#include <mqtt/async_client.h>
#include <mqtt/callback.h>

namespace pbt {

class MqttTcpReceiver final : public Receiver, private mqtt::callback {
public:
    MqttTcpReceiver(const BenchmarkConfig& cfg,
                    BoundedBlockingQueue<InboundPacket>& queue,
                    int64_t ntp_offset_ns = 0);
    std::expected<void, Error> bind()  override;
    std::expected<void, Error> start() override;
    std::expected<void, Error> stop()  override;
private:
    void message_arrived(mqtt::const_message_ptr msg) override;
    mqtt::async_client client_;
};

}  // namespace pbt
