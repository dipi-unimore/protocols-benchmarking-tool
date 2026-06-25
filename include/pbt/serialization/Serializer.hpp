#pragma once
#include "pbt/core/Types.hpp"
#include <expected>
#include <memory>
#include <span>

namespace pbt {

class Serializer {
public:
    virtual ~Serializer() = default;

    [[nodiscard]] virtual std::expected<Bytes, Error>
        serialize(std::span<const uint8_t> payload) = 0;

    [[nodiscard]] virtual std::expected<Bytes, Error>
        deserialize(std::span<const uint8_t> data) = 0;

    [[nodiscard]] virtual SerFmt format() const noexcept = 0;

    static std::unique_ptr<Serializer> create(SerFmt fmt,
                                               const std::string& source_format = "unknown");
};

}  // namespace pbt
