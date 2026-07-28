#pragma once
#include "Sender.hpp"

namespace pbt {

class UdpSender final : public Sender {
public:
    UdpSender(const BenchmarkConfig& cfg, std::unique_ptr<PayloadSource> src,
              std::unique_ptr<Serializer> ser, std::unique_ptr<Compressor> cmp,
              NtpInfo ntp_info = {});
    ~UdpSender() override;
    std::expected<void, Error> connect()    override;
    std::expected<void, Error> disconnect() override;
protected:
    std::expected<void, Error> do_send(std::span<const uint8_t> wire) override;
private:
    std::expected<void, Error> send_datagram(std::span<const uint8_t> data);
    int fd_{-1};
};

}  // namespace pbt
