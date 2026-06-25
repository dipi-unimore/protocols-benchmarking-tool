#pragma once
#include "pbt/core/BenchmarkConfig.hpp"
#include "pbt/core/Crc32.hpp"
#include "pbt/core/Types.hpp"
#include "pbt/core/WireHeader.hpp"
#include <expected>
#include <memory>
#include <span>

namespace pbt {

class PayloadSource;
class Serializer;
class Compressor;

class Sender {
public:
    Sender(const BenchmarkConfig& cfg,
           std::unique_ptr<PayloadSource> src,
           std::unique_ptr<Serializer>    ser,
           std::unique_ptr<Compressor>    cmp,
           int64_t ntp_offset_ns = 0);
    virtual ~Sender() = default;

    // Stamps all send-side timestamps; ts_sent stamped BEFORE assemble+do_send.
    [[nodiscard]] std::expected<WireHeader, Error> send(uint64_t seq_id,
                                                         bool is_warmup = false);

    [[nodiscard]] std::expected<void, Error> send_sentinel(uint64_t last_seq_id);

    virtual std::expected<void, Error> connect()    = 0;
    virtual std::expected<void, Error> disconnect() = 0;

    static std::unique_ptr<Sender> create(const BenchmarkConfig& cfg,
                                           std::unique_ptr<PayloadSource> src,
                                           std::unique_ptr<Serializer>    ser,
                                           std::unique_ptr<Compressor>    cmp,
                                           int64_t ntp_offset_ns = 0);

protected:
    virtual std::expected<void, Error> do_send(std::span<const uint8_t> wire) = 0;

    [[nodiscard]] static Bytes assemble_wire(const WireHeader& hdr,
                                              std::span<const uint8_t> payload);

    const BenchmarkConfig&         config_;
    std::unique_ptr<PayloadSource> source_;
    std::unique_ptr<Serializer>    serializer_;
    std::unique_ptr<Compressor>    compressor_;
    int64_t                        ntp_offset_ns_{0};
};

}  // namespace pbt
