#pragma once
#include "PacketMetrics.hpp"
#include "WelfordAccumulator.hpp"
#include "pbt/core/BoundedBlockingQueue.hpp"
#include "pbt/core/InboundPacket.hpp"
#include "pbt/core/RunResult.hpp"
#include <optional>
#include <thread>
#include <vector>

namespace pbt {

class Serializer;
class Compressor;
class CsvPacketWriter;

class PacketProcessor {
public:
    PacketProcessor(BoundedBlockingQueue<InboundPacket>& queue,
                    Serializer&        ser,
                    Compressor&        cmp,
                    CsvPacketWriter&   writer,
                    const BenchmarkConfig& cfg);

    void start();
    void stop();

    void set_overflow_count(uint64_t n) noexcept          { overflow_count_ = n; }
    void set_fragment_timeout_count(uint64_t n) noexcept  { fragment_timeout_count_ = n; }
    void set_msgs_sent(std::optional<uint64_t> n) noexcept { msgs_sent_ = n; }

    [[nodiscard]] RunResult finalize() const;

private:
    void process_loop(std::stop_token st);
    PacketMetrics compute(InboundPacket& pkt);

    BoundedBlockingQueue<InboundPacket>& queue_;
    Serializer&      ser_;
    Compressor&      cmp_;
    CsvPacketWriter& writer_;
    const BenchmarkConfig& cfg_;
    std::jthread     thread_;

    // Sequential state — no mutex needed (single processing thread)
    uint64_t last_seq_id_{0};
    bool     first_packet_{true};
    double   last_e2e_delay_us_{0.0};

    WelfordAccumulator e2e_acc_, jitter_acc_, ser_acc_, cmp_acc_,
                       decomp_acc_, deser_acc_, proc_acc_;

    // Algorithm R reservoir for e2e percentiles (capped at 100k)
    std::vector<double> reservoir_e2e_us_;
    uint64_t            reservoir_count_{0};

    uint64_t msgs_received_{0};
    uint64_t out_of_order_{0};
    uint64_t duplicates_{0};
    uint64_t crc_errors_{0};
    uint64_t total_seq_gap_{0};
    uint64_t overflow_count_{0};
    uint64_t fragment_timeout_count_{0};
    std::optional<uint64_t> msgs_sent_;

    static constexpr std::size_t kReservoirCap = 100'000;
};

}  // namespace pbt
