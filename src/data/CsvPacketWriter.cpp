#include "pbt/data/CsvPacketWriter.hpp"
#include <format>
#include <stdexcept>

namespace pbt {

std::string CsvPacketWriter::header() {
    return "run_id,protocol,serializer,compression,zstd_dict,crc_ok,"
           "seq_id,payload_size_bytes,wire_size_bytes,"
           "ts_created_ns,ts_serialized_ns,ts_compressed_ns,ts_sent_ns,"
           "ts_received_ns,ts_decompressed_ns,ts_deserialized_ns,ts_processed_ns,"
           "ntp_offset_sender_ns,ntp_offset_receiver_ns,"
           "e2e_delay_us,serialization_us,compression_us,transport_us,"
           "decompression_us,deserialization_us,processing_us,"
           "jitter_us,is_out_of_order,is_duplicate,seq_gap\n";
}

CsvPacketWriter::CsvPacketWriter(const std::filesystem::path& path, CsvRowContext ctx)
    : out_(path, std::ios::out | std::ios::trunc), ctx_(std::move(ctx)) {
    if (!out_) throw std::runtime_error("Cannot open CSV: " + path.string());
    out_ << header();
}

CsvPacketWriter::~CsvPacketWriter() {
    out_.flush();
}

void CsvPacketWriter::write_row(const PacketMetrics& m) {
    std::lock_guard<std::mutex> lk(mu_);
    out_ << std::format("{},{},{},{},{},{},",
                        ctx_.run_id, ctx_.protocol, ctx_.serializer,
                        ctx_.compression, ctx_.zstd_dict,
                        m.crc_ok ? 1 : 0);
    out_ << std::format("{},{},{},",
                        m.seq_id, m.payload_size_bytes, m.wire_size_bytes);
    out_ << std::format("{},{},{},{},",
                        m.ts_created_ns, m.ts_serialized_ns,
                        m.ts_compressed_ns, m.ts_sent_ns);
    out_ << std::format("{},{},{},{},",
                        m.ts_received_ns, m.ts_decompressed_ns,
                        m.ts_deserialized_ns, m.ts_processed_ns);
    out_ << std::format("{},{},",
                        m.ntp_offset_sender_ns, m.ntp_offset_receiver_ns);
    out_ << std::format("{:.3f},{:.3f},{:.3f},{:.3f},",
                        m.e2e_delay_us, m.serialization_us,
                        m.compression_us, m.transport_us);
    out_ << std::format("{:.3f},{:.3f},{:.3f},",
                        m.decompression_us, m.deserialization_us, m.processing_us);
    out_ << std::format("{:.3f},{},{},{}\n",
                        m.jitter_us,
                        m.is_out_of_order ? 1 : 0,
                        m.is_duplicate    ? 1 : 0,
                        m.seq_gap);
    if (++rows_since_flush_ >= kFlushEvery) {
        out_.flush();
        rows_since_flush_ = 0;
    }
}

void CsvPacketWriter::flush() {
    std::lock_guard<std::mutex> lk(mu_);
    out_.flush();
}

}  // namespace pbt
