#pragma once
#include "pbt/core/BenchmarkConfig.hpp"
#include "pbt/core/BoundedBlockingQueue.hpp"
#include "pbt/core/InboundPacket.hpp"
#include "pbt/core/Types.hpp"
#include <atomic>
#include <expected>
#include <memory>
#include <span>

namespace pbt {

class Receiver {
public:
    Receiver(const BenchmarkConfig& cfg,
             BoundedBlockingQueue<InboundPacket>& queue,
             NtpInfo ntp_info = {});
    virtual ~Receiver() = default;

    virtual std::expected<void, Error> bind()  = 0;
    virtual std::expected<void, Error> start() = 0;
    virtual std::expected<void, Error> stop()  = 0;

    [[nodiscard]] uint64_t overflow_count() const noexcept {
        return overflow_count_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] bool     sentinel_received() const noexcept { return sentinel_received_; }
    [[nodiscard]] uint64_t msgs_sent_from_sentinel() const noexcept {
        return msgs_sent_from_sentinel_;
    }

    static std::unique_ptr<Receiver> create(const BenchmarkConfig& cfg,
                                             BoundedBlockingQueue<InboundPacket>& queue,
                                             NtpInfo ntp_info = {});

protected:
    void on_wire_bytes(std::span<const uint8_t> wire);

    static bool parse_wire(std::span<const uint8_t> wire,
                           WireHeader& hdr_out,
                           std::span<const uint8_t>& payload_out);

    const BenchmarkConfig&               config_;
    BoundedBlockingQueue<InboundPacket>& queue_;
    NtpInfo                               ntp_info_{};
    std::atomic<uint64_t>               overflow_count_{0};
    bool                                 sentinel_received_{false};
    uint64_t                             msgs_sent_from_sentinel_{0};
};

}  // namespace pbt
