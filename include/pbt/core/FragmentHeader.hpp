#pragma once
#include <cstdint>

namespace pbt {

// 24-byte header prepended to each UDP datagram fragment.
// Fragment 0: [FragmentHeader][WireHeader][data...]
// Fragment N: [FragmentHeader][data...]
struct FragmentHeader {
    uint32_t magic{0x50425446u};   // 'PBTF'
    uint64_t message_id{0};        // matches WireHeader.sequence_id
    uint16_t frag_index{0};        // 0-based
    uint16_t frag_count{0};        // total fragments for this message
    uint32_t frag_data_size{0};    // payload bytes in this datagram
};
static_assert(sizeof(FragmentHeader) == 24, "FragmentHeader must be exactly 24 bytes");

}  // namespace pbt
