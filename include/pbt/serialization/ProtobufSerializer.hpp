#pragma once
#include "Serializer.hpp"
#include <string>

namespace pbt {

// Wraps payload bytes in pbt::PbtPayload protobuf message.
class ProtobufSerializer final : public Serializer {
public:
    explicit ProtobufSerializer(std::string source_format = "unknown");
    std::expected<Bytes, Error> serialize(std::span<const uint8_t> payload) override;
    std::expected<Bytes, Error> deserialize(std::span<const uint8_t> data) override;
    SerFmt format() const noexcept override { return SerFmt::Protobuf; }
private:
    std::string source_format_;
};

}  // namespace pbt
