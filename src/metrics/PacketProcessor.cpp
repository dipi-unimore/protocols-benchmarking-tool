#include "pbt/metrics/PacketProcessor.hpp"
#include "pbt/metrics/Statistics.hpp"
#include "pbt/core/Crc32.hpp"
#include "pbt/core/WireHeader.hpp"
#include "pbt/data/CsvPacketWriter.hpp"
#include "pbt/serialization/Serializer.hpp"
#include "pbt/compression/Compressor.hpp"
#include <random>

namespace pbt {

PacketProcessor::PacketProcessor(BoundedBlockingQueue<InboundPacket>& queue,
                                  Serializer&        ser,
                                  Compressor&        cmp,
                                  CsvPacketWriter&   writer,
                                  const BenchmarkConfig& cfg)
    : queue_(queue), ser_(ser), cmp_(cmp), writer_(writer), cfg_(cfg) {
    reservoir_e2e_us_.reserve(kReservoirCap);
}

void PacketProcessor::start() {
    thread_ = std::jthread([this](std::stop_token st) { process_loop(st); });
}

void PacketProcessor::stop() {
    thread_.request_stop();
    queue_.stop();
}

PacketMetrics PacketProcessor::compute(InboundPacket& pkt) {
    PacketMetrics m;
    m.seq_id        = pkt.header.sequence_id;
    m.is_warmup     = pkt.header.is_warmup();
    m.wire_size_bytes = static_cast<uint32_t>(
        sizeof(WireHeader) + pkt.wire_payload.size());
    m.payload_size_bytes = pkt.header.original_size;

    // Copy timestamps from header
    m.ts_created_ns      = pkt.header.ts_created_ns;
    m.ts_serialized_ns   = pkt.header.ts_serialized_ns;
    m.ts_compressed_ns   = pkt.header.ts_compressed_ns;
    m.ts_sent_ns         = pkt.header.ts_sent_ns;
    m.ntp_offset_sender_ns   = pkt.header.ntp_offset_ns;
    m.ntp_offset_receiver_ns = pkt.receiver_ntp_offset_ns;

    m.ts_received_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                           pkt.ts_received.time_since_epoch()).count();

    // CRC32 validation
    m.crc_ok = (crc32(pkt.wire_payload.data(), pkt.wire_payload.size())
                == pkt.header.payload_crc32);
    if (!m.crc_ok) {
        ++crc_errors_;
        // Still compute timing from header; skip decompress/deserialize
        m.ts_processed_ns = now_ns();
        return m;
    }

    // Decompress
    auto decomp = cmp_.decompress(pkt.wire_payload, pkt.header.original_size);
    m.ts_decompressed_ns = now_ns();
    if (!decomp) { m.ts_processed_ns = now_ns(); return m; }

    // Deserialize
    auto deser = ser_.deserialize(*decomp);
    m.ts_deserialized_ns = now_ns();
    if (!deser) { m.ts_processed_ns = now_ns(); return m; }

    m.ts_processed_ns = now_ns();

    // Derived latencies (µs)
    m.serialization_us = ns_to_us(m.ts_serialized_ns  - m.ts_created_ns);
    m.compression_us   = ns_to_us(m.ts_compressed_ns  - m.ts_serialized_ns);
    m.transport_us     = ns_to_us(m.ts_sent_ns         - m.ts_compressed_ns);
    m.decompression_us = ns_to_us(m.ts_decompressed_ns - m.ts_received_ns);
    m.deserialization_us = ns_to_us(m.ts_deserialized_ns - m.ts_decompressed_ns);
    m.processing_us    = ns_to_us(m.ts_processed_ns   - m.ts_deserialized_ns);

    m.e2e_delay_us = compute_e2e_delay_us(m.ts_received_ns, m.ts_sent_ns,
                                           m.ntp_offset_sender_ns, m.ntp_offset_receiver_ns);

    // OOO / duplicate / gap detection
    if (first_packet_) {
        first_packet_  = false;
        last_seq_id_   = m.seq_id;
        m.seq_gap      = 0;
    } else {
        if (m.seq_id < last_seq_id_) {
            m.is_out_of_order = true;
            ++out_of_order_;
        } else if (m.seq_id == last_seq_id_) {
            m.is_duplicate = true;
            ++duplicates_;
        } else {
            m.seq_gap = static_cast<int64_t>(m.seq_id - last_seq_id_ - 1);
            total_seq_gap_ += static_cast<uint64_t>(m.seq_gap);
            last_seq_id_ = m.seq_id;
        }
    }

    m.jitter_us = m.is_warmup ? 0.0 : std::abs(m.e2e_delay_us - last_e2e_delay_us_);
    if (!m.is_warmup) last_e2e_delay_us_ = m.e2e_delay_us;

    return m;
}

void PacketProcessor::process_loop(std::stop_token st) {
    std::mt19937_64 rng{std::random_device{}()};

    while (true) {
        auto opt = queue_.pop();
        if (!opt) break;
        auto m = compute(*opt);
        writer_.write_row(m);
        ++msgs_received_;

        if (m.is_warmup) continue;  // warmup excluded from statistics

        // Update Welford accumulators
        if (m.crc_ok) {
            e2e_acc_.update(m.e2e_delay_us);
            jitter_acc_.update(m.jitter_us);
            ser_acc_.update(m.serialization_us);
            cmp_acc_.update(m.compression_us);
            decomp_acc_.update(m.decompression_us);
            deser_acc_.update(m.deserialization_us);
            proc_acc_.update(m.processing_us);

            // Algorithm R reservoir sampling
            ++reservoir_count_;
            if (reservoir_e2e_us_.size() < kReservoirCap) {
                reservoir_e2e_us_.push_back(m.e2e_delay_us);
            } else {
                std::uniform_int_distribution<uint64_t> dist(0, reservoir_count_ - 1);
                auto j = dist(rng);
                if (j < kReservoirCap)
                    reservoir_e2e_us_[j] = m.e2e_delay_us;
            }
        }
    }
}

RunResult PacketProcessor::finalize() const {
    RunResult r;
    r.config                  = cfg_;
    r.msgs_sent               = msgs_sent_;
    r.msgs_received           = msgs_received_;
    r.out_of_order_count      = out_of_order_;
    r.duplicate_count         = duplicates_;
    r.crc_error_count         = crc_errors_;
    r.total_seq_gap           = total_seq_gap_;
    r.overflow_count          = overflow_count_;
    r.fragment_timeout_count  = fragment_timeout_count_;

    if (msgs_sent_.has_value() && *msgs_sent_ > 0) {
        uint64_t sent = *msgs_sent_;
        r.packet_loss_pct = 100.0 * (1.0 - static_cast<double>(msgs_received_)
                                          / static_cast<double>(sent));
    }

    // Percentiles from reservoir
    auto reservoir_copy = reservoir_e2e_us_;
    r.e2e = stats::compute(reservoir_copy);

    // Other latencies from Welford only (no percentiles beyond mean/stddev)
    auto fill = [](LatencyStats& ls, const WelfordAccumulator& acc) {
        ls.mean_us   = acc.mean();
        ls.stddev_us = acc.stddev();
        ls.min_us    = acc.min();
        ls.max_us    = acc.max();
    };
    fill(r.serialization,   ser_acc_);
    fill(r.compression,     cmp_acc_);
    fill(r.decompression,   decomp_acc_);
    fill(r.deserialization, deser_acc_);

    return r;
}

}  // namespace pbt
