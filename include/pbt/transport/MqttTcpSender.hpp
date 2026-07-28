#pragma once
#include "Sender.hpp"
#include <mqtt/async_client.h>

namespace pbt {

class MqttTcpSender final : public Sender {
public:
    MqttTcpSender(const BenchmarkConfig& cfg, std::unique_ptr<PayloadSource> src,
                  std::unique_ptr<Serializer> ser, std::unique_ptr<Compressor> cmp,
                  NtpInfo ntp_info = {});
    std::expected<void, Error> connect()    override;
    std::expected<void, Error> disconnect() override;
protected:
    std::expected<void, Error> do_send(std::span<const uint8_t> wire) override;
private:
    mqtt::async_client client_;
};

}  // namespace pbt
