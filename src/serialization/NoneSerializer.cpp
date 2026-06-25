#include "pbt/serialization/NoneSerializer.hpp"

namespace pbt {

std::expected<Bytes, Error>
NoneSerializer::serialize(std::span<const uint8_t> p) {
    return Bytes(p.begin(), p.end());
}

std::expected<Bytes, Error>
NoneSerializer::deserialize(std::span<const uint8_t> d) {
    return Bytes(d.begin(), d.end());
}

}  // namespace pbt
