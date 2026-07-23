#pragma once
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace pbt {

enum class Protocol    : uint8_t { MqttTcp = 0, ZmqTcp = 1, Tcp = 2, Udp = 3 };
enum class SerFmt      : uint8_t { None = 0, Cbor = 1, Protobuf = 2 };
enum class Compression : uint8_t { None = 0, Zstd = 1 };
enum class QoS         : uint8_t { AtMostOnce = 0, AtLeastOnce = 1, ExactlyOnce = 2 };
enum class ZmqPattern  : uint8_t { PushPull = 0, PubSub = 1 };
enum class PayloadFmt  : uint8_t { Text = 0, Json = 1, Yaml = 2, Kv = 3, Binary = 4, Random = 5 };

using Bytes     = std::vector<uint8_t>;
// system_clock is the only standard clock guaranteed to be Unix-epoch-based,
// which cross-machine NTP offset correction requires. high_resolution_clock
// is implementation-defined and aliases steady_clock (boot-time epoch) on
// libstdc++/Linux while aliasing system_clock on libc++/macOS, silently
// breaking cross-machine timestamp comparison.
using Clock     = std::chrono::system_clock;
using TimePoint = Clock::time_point;
using Duration  = std::chrono::nanoseconds;

struct Error {
    std::string message;
    int         code{0};
};

[[nodiscard]] inline int64_t now_ns() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               Clock::now().time_since_epoch()).count();
}

[[nodiscard]] inline double ns_to_us(int64_t ns) noexcept {
    return static_cast<double>(ns) / 1000.0;
}

}  // namespace pbt
