#pragma once
#include "pbt/core/Types.hpp"
#include <chrono>
#include <expected>
#include <string>

namespace pbt {

class NtpSync {
public:
    // RFC 4330 SNTP query. Returns clock offset in nanoseconds.
    // Hard error if all retries fail.
    [[nodiscard]] static std::expected<int64_t, Error>
        query(const std::string& server = "pool.ntp.org",
              int retries = 3,
              std::chrono::milliseconds timeout = std::chrono::milliseconds{3000});
};

}  // namespace pbt
