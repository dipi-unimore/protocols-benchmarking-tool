#pragma once
#include "pbt/core/Types.hpp"
#include <chrono>
#include <expected>
#include <string>

namespace pbt {

class NtpSync {
public:
    // RFC 4330 SNTP query. Takes `samples` independent exchanges and keeps the
    // minimum-RTT one (lowest RTT correlates with the most symmetric path, and
    // therefore the least offset error — standard NTP client practice). This
    // matters because a single-shot SNTP exchange over a real network path has
    // an offset error proportional to path asymmetry, typically hundreds of
    // microseconds to several milliseconds against a public server — an error
    // that can dominate the very transport delay the tool is trying to measure
    // on localhost/LAN runs. Hard error only if every sample fails.
    [[nodiscard]] static std::expected<NtpInfo, Error>
        query(const std::string& server = "pool.ntp.org",
              int samples = 8,
              std::chrono::milliseconds timeout = std::chrono::milliseconds{3000});
};

}  // namespace pbt
