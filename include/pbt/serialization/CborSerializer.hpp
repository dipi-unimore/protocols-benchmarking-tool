#pragma once
#include "Serializer.hpp"

namespace pbt {

// CBOR re-encodes structured (JSON) payloads to binary CBOR.
// Input must be valid JSON bytes. Valid with Json, Yaml, Kv payload formats only.
class CborSerializer final : public Serializer {
public:
    std::expected<Bytes, Error> serialize(std::span<const uint8_t> payload) override;
    std::expected<Bytes, Error> deserialize(std::span<const uint8_t> data) override;
    SerFmt format() const noexcept override { return SerFmt::Cbor; }
};

}  // namespace pbt
