#pragma once
#include "Compressor.hpp"

namespace pbt {

class NoopCompressor final : public Compressor {
public:
    std::expected<Bytes, Error> compress(std::span<const uint8_t> in) override;
    std::expected<Bytes, Error> decompress(std::span<const uint8_t> in, std::size_t) override;
    Compression type() const noexcept override { return Compression::None; }
};

}  // namespace pbt
