#include "pbt/data/CsvSenderWriter.hpp"
#include <format>
#include <stdexcept>

namespace pbt {

std::string CsvSenderWriter::header() {
    return "run_id,seq_id,is_warmup,"
           "payload_size_bytes,wire_size_bytes,"
           "ts_created_ns,ts_serialized_ns,ts_compressed_ns,ts_sent_ns,"
           "ntp_offset_ns,crc32\n";
}

CsvSenderWriter::CsvSenderWriter(const std::filesystem::path& path)
    : out_(path, std::ios::out | std::ios::trunc) {
    if (!out_) throw std::runtime_error("Cannot open sender CSV: " + path.string());
    out_ << header();
}

CsvSenderWriter::~CsvSenderWriter() {
    out_.flush();
}

void CsvSenderWriter::write_row(const WireHeader& hdr, uint32_t payload_size_bytes,
                                 uint32_t wire_size_bytes) {
    out_ << std::format(",{},{},",
                        hdr.sequence_id,
                        hdr.is_warmup() ? 1 : 0);
    out_ << std::format("{},{},",
                        payload_size_bytes, wire_size_bytes);
    out_ << std::format("{},{},{},{},",
                        hdr.ts_created_ns, hdr.ts_serialized_ns,
                        hdr.ts_compressed_ns, hdr.ts_sent_ns);
    out_ << std::format("{},{}\n",
                        hdr.ntp_offset_ns, hdr.payload_crc32);
    if (++rows_since_flush_ >= kFlushEvery) {
        out_.flush();
        rows_since_flush_ = 0;
    }
}

void CsvSenderWriter::flush() {
    out_.flush();
}

}  // namespace pbt
