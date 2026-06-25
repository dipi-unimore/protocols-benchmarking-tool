#pragma once
#include <cstdint>

namespace pbt {

// Fixed 72-byte binary header prepended to every Message.
// All fields naturally aligned — no pragma pack needed.
struct WireHeader {
    // offset  0 — control (8 bytes)
    uint32_t magic{0x50425401u};    // 'PBT1'
    uint8_t  version{1};
    uint8_t  flags{0};              // bit0=sentinel, bit1=fragment, bit2=warmup
    uint8_t  serializer_id{0};
    uint8_t  compressor_id{0};
    // offset  8 — identity (8 bytes)
    uint64_t sequence_id{0};
    // offset 16 — sizes (8 bytes)
    uint32_t payload_size{0};       // compressed payload bytes after header
    uint32_t original_size{0};      // uncompressed payload bytes
    // offset 24 — integrity & protocol (8 bytes)
    uint8_t  qos{0};
    uint8_t  padding[3]{};
    uint32_t payload_crc32{0};      // CRC32 of payload bytes; validated before decompression
    // offset 32 — send-side timestamps ns since epoch (40 bytes)
    int64_t  ts_created_ns{0};
    int64_t  ts_serialized_ns{0};
    int64_t  ts_compressed_ns{0};
    int64_t  ts_sent_ns{0};
    int64_t  ntp_offset_ns{0};      // sender's SNTP offset at startup

    [[nodiscard]] bool is_sentinel() const noexcept { return (flags & 0x01u) != 0; }
    [[nodiscard]] bool is_fragment() const noexcept { return (flags & 0x02u) != 0; }
    [[nodiscard]] bool is_warmup()   const noexcept { return (flags & 0x04u) != 0; }

    void set_sentinel() noexcept { flags |= 0x01u; }
    void set_fragment() noexcept { flags |= 0x02u; }
    void set_warmup()   noexcept { flags |= 0x04u; }
};
static_assert(sizeof(WireHeader) == 72, "WireHeader must be exactly 72 bytes");

}  // namespace pbt
