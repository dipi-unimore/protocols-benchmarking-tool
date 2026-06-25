#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace pbt {

namespace detail {
constexpr std::array<uint32_t, 256> make_crc_table() noexcept {
    std::array<uint32_t, 256> t{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int j = 0; j < 8; ++j)
            c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        t[i] = c;
    }
    return t;
}
inline constexpr auto kCrcTable = make_crc_table();
}  // namespace detail

[[nodiscard]] inline uint32_t crc32(std::span<const uint8_t> data) noexcept {
    uint32_t crc = 0xFFFFFFFFu;
    for (uint8_t b : data)
        crc = detail::kCrcTable[(crc ^ b) & 0xFFu] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

[[nodiscard]] inline uint32_t crc32(const uint8_t* data, std::size_t len) noexcept {
    return crc32(std::span<const uint8_t>(data, len));
}

}  // namespace pbt
