#pragma once
#include "Serializer.hpp"

namespace pbt {

class NoneSerializer final : public Serializer {
public:
    std::expected<Bytes, Error> serialize(std::span<const uint8_t> p) override;
    std::expected<Bytes, Error> deserialize(std::span<const uint8_t> d) override;
    SerFmt format() const noexcept override { return SerFmt::None; }
};

}  // namespace pbt
