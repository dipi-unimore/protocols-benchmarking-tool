#pragma once
#include "PayloadSource.hpp"
#include <filesystem>
#include <vector>

namespace pbt {

// Base for file-backed sources; loads once at construction, cycles in memory.
class FileSource : public PayloadSource {
public:
    std::span<const uint8_t> next() noexcept override {
        auto& rec = records_[idx_];
        idx_ = (idx_ + 1) % records_.size();
        return rec;
    }
    std::size_t record_count() const noexcept override { return records_.size(); }

protected:
    std::vector<Bytes> records_;
    std::size_t        idx_{0};
};

class PlainTextSource final : public FileSource {
public:
    explicit PlainTextSource(const std::filesystem::path& path);
};

class JsonSource final : public FileSource {
public:
    explicit JsonSource(const std::filesystem::path& path);
};

class YamlSource final : public FileSource {
public:
    explicit YamlSource(const std::filesystem::path& path);
};

class KeyValueSource final : public FileSource {
public:
    explicit KeyValueSource(const std::filesystem::path& path);
};

class BinarySource final : public PayloadSource {
public:
    explicit BinarySource(const std::filesystem::path& path);
    std::span<const uint8_t> next() noexcept override { return buf_; }
    std::size_t record_count() const noexcept override { return 1; }
private:
    Bytes buf_;
};

}  // namespace pbt
