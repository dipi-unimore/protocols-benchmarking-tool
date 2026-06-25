#pragma once
#include "pbt/core/FragmentHeader.hpp"
#include "pbt/core/Types.hpp"
#include <chrono>
#include <functional>
#include <span>
#include <unordered_map>
#include <vector>

namespace pbt {

class ReassemblyBuffer {
public:
    using ReassembledCallback = std::function<void(Bytes)>;

    explicit ReassemblyBuffer(
        std::chrono::milliseconds expiry_ms = std::chrono::milliseconds{500});

    // Insert fragment; calls cb when message fully reassembled. Returns expired count.
    std::size_t insert(const FragmentHeader& fhdr, std::span<const uint8_t> data,
                       const ReassembledCallback& cb);

    [[nodiscard]] std::size_t timeout_count() const noexcept { return timeout_count_; }

private:
    struct Slot {
        std::vector<Bytes>                    frags;
        uint16_t                              received{0};
        uint16_t                              total{0};
        std::chrono::steady_clock::time_point created;
        [[nodiscard]] bool complete() const noexcept { return received == total; }
    };

    void sweep_expired();

    std::chrono::milliseconds                  expiry_ms_;
    std::unordered_map<uint64_t, Slot>         slots_;
    std::size_t                                timeout_count_{0};
};

}  // namespace pbt
