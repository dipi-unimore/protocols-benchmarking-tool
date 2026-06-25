#pragma once
#include "pbt/metrics/PacketMetrics.hpp"
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

namespace pbt {

struct CsvRowContext {
    std::string run_id;
    std::string protocol;
    std::string serializer;
    std::string compression;
    std::string zstd_dict;  // empty if none
};

class CsvPacketWriter {
public:
    CsvPacketWriter(const std::filesystem::path& path, CsvRowContext ctx);
    ~CsvPacketWriter();

    void write_row(const PacketMetrics& m);
    void flush();

    [[nodiscard]] static std::string header();

private:
    std::ofstream out_;
    std::mutex    mu_;
    CsvRowContext ctx_;
    uint32_t      rows_since_flush_{0};
    static constexpr uint32_t kFlushEvery = 64;
};

}  // namespace pbt
