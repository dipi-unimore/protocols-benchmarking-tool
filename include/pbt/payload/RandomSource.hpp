#pragma once
#include "PayloadSource.hpp"

namespace pbt {

class RandomSource final : public PayloadSource {
public:
    explicit RandomSource(std::size_t size_bytes = 256);
    std::span<const uint8_t> next() noexcept override { return buf_; }
    std::size_t record_count() const noexcept override { return 1; }
private:
    Bytes buf_;
};

}  // namespace pbt
