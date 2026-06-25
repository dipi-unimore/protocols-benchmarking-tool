#pragma once
#include "pbt/core/WireHeader.hpp"
#include <filesystem>
#include <fstream>
#include <string>

namespace pbt {

class CsvSenderWriter {
public:
    explicit CsvSenderWriter(const std::filesystem::path& path);
    ~CsvSenderWriter();

    void write_row(const WireHeader& hdr, uint32_t payload_size_bytes,
                   uint32_t wire_size_bytes);
    void flush();

    [[nodiscard]] static std::string header();

private:
    std::ofstream out_;
    uint32_t      rows_since_flush_{0};
    static constexpr uint32_t kFlushEvery = 64;
};

}  // namespace pbt
