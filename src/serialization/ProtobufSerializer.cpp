#include "pbt/serialization/ProtobufSerializer.hpp"
#include "pbt/serialization/NoneSerializer.hpp"
#include "pbt/serialization/CborSerializer.hpp"
#include "pbt_message.pb.h"
#include <format>

namespace pbt {

ProtobufSerializer::ProtobufSerializer(std::string source_format)
    : source_format_(std::move(source_format)) {}

std::expected<Bytes, Error>
ProtobufSerializer::serialize(std::span<const uint8_t> payload) {
    pbt::PbtPayload msg;
    msg.set_data(payload.data(), payload.size());
    msg.set_source_format(source_format_);
    Bytes out(msg.ByteSizeLong());
    if (!msg.SerializeToArray(out.data(), static_cast<int>(out.size())))
        return std::unexpected(Error{"Protobuf serialize failed"});
    return out;
}

std::expected<Bytes, Error>
ProtobufSerializer::deserialize(std::span<const uint8_t> data) {
    pbt::PbtPayload msg;
    if (!msg.ParseFromArray(data.data(), static_cast<int>(data.size())))
        return std::unexpected(Error{"Protobuf parse failed"});
    const auto& d = msg.data();
    return Bytes(d.begin(), d.end());
}

// --- factory ---

std::unique_ptr<Serializer> Serializer::create(SerFmt fmt,
                                                const std::string& source_format) {
    switch (fmt) {
        case SerFmt::None:     return std::make_unique<NoneSerializer>();
        case SerFmt::Cbor:     return std::make_unique<CborSerializer>();
        case SerFmt::Protobuf: return std::make_unique<ProtobufSerializer>(source_format);
    }
    return std::make_unique<NoneSerializer>();
}

}  // namespace pbt
