#pragma once
#include "pbt/core/Types.hpp"
#include <expected>
#include <memory>
#include <span>
#include <string>

namespace pbt {

class Compressor {
public:
    virtual ~Compressor() = default;

    [[nodiscard]] virtual std::expected<Bytes, Error>
        compress(std::span<const uint8_t> in) = 0;

    [[nodiscard]] virtual std::expected<Bytes, Error>
        decompress(std::span<const uint8_t> in, std::size_t original_size) = 0;

    [[nodiscard]] virtual Compression type() const noexcept = 0;

    static std::unique_ptr<Compressor> create(Compression c,
                                               const std::string& dict_path = "");
};

}  // namespace pbt
