#include "pbt/transport/ReassemblyBuffer.hpp"
#include <numeric>

namespace pbt {

ReassemblyBuffer::ReassemblyBuffer(std::chrono::milliseconds expiry_ms)
    : expiry_ms_(expiry_ms) {}

std::size_t ReassemblyBuffer::insert(const FragmentHeader& fhdr,
                                      std::span<const uint8_t> data,
                                      const ReassembledCallback& cb) {
    auto& slot = slots_[fhdr.message_id];
    if (slot.total == 0) {
        slot.total   = fhdr.frag_count;
        slot.frags.resize(fhdr.frag_count);
        slot.created = std::chrono::steady_clock::now();
    }

    if (fhdr.frag_index < slot.total && slot.frags[fhdr.frag_index].empty()) {
        slot.frags[fhdr.frag_index] = Bytes(data.begin(), data.end());
        ++slot.received;
    }

    std::size_t swept = 0;
    if (slot.complete()) {
        Bytes msg;
        msg.reserve(std::accumulate(slot.frags.begin(), slot.frags.end(),
                                     std::size_t{0},
                                     [](std::size_t a, const Bytes& b) { return a + b.size(); }));
        for (auto& frag : slot.frags)
            msg.insert(msg.end(), frag.begin(), frag.end());
        slots_.erase(fhdr.message_id);
        sweep_expired();
        cb(std::move(msg));
    }
    return swept;
}

void ReassemblyBuffer::sweep_expired() {
    auto now = std::chrono::steady_clock::now();
    for (auto it = slots_.begin(); it != slots_.end(); ) {
        if (now - it->second.created > expiry_ms_) {
            ++timeout_count_;
            it = slots_.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace pbt
