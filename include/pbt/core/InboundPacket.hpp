#pragma once
#include "Types.hpp"
#include "WireHeader.hpp"

namespace pbt {

struct InboundPacket {
    WireHeader header;                  // parsed from wire; on stack
    Bytes      wire_payload;            // compressed+serialized bytes after header
    TimePoint  ts_received;             // stamped in network thread before enqueue
    int64_t    receiver_ntp_offset_ns{0};
};

}  // namespace pbt
