#pragma once
#include "Sender.hpp"

namespace pbt {

class TcpSender final : public Sender {
public:
    TcpSender(const BenchmarkConfig& cfg, std::unique_ptr<PayloadSource> src,
              std::unique_ptr<Serializer> ser, std::unique_ptr<Compressor> cmp,
              NtpInfo ntp_info = {});
    ~TcpSender() override;
    std::expected<void, Error> connect()    override;
    std::expected<void, Error> disconnect() override;
protected:
    std::expected<void, Error> do_send(std::span<const uint8_t> wire) override;
private:
    int fd_{-1};
};

}  // namespace pbt
