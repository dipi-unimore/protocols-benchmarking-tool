#pragma once
#include "Types.hpp"
#include <expected>
#include <filesystem>
#include <map>
#include <nlohmann/json.hpp>
#include <string>

namespace pbt {

struct BenchmarkConfig {
    Protocol    protocol{Protocol::Tcp};
    std::string host{"localhost"};
    uint16_t    port{9000};

    SerFmt      serializer{SerFmt::None};
    Compression compression{Compression::None};
    std::string zstd_dict_path;

    std::string payload_file;
    PayloadFmt  payload_format{PayloadFmt::Random};
    uint32_t    payload_size_bytes{256};

    uint32_t message_rate_hz{100};
    uint32_t duration_s{30};
    uint32_t warmup_s{5};

    std::string mqtt_topic{"pbt/bench"};
    QoS         mqtt_qos{QoS::AtMostOnce};
    ZmqPattern  zmq_pattern{ZmqPattern::PushPull};
    std::string zmq_topic;

    uint32_t receiver_timeout_extra_s{5};
    uint32_t udp_mtu{1472};

    std::map<std::string, std::string> metadata;

    [[nodiscard]] std::expected<void, Error> validate() const;

    [[nodiscard]] nlohmann::json to_json() const;
    static std::expected<BenchmarkConfig, Error> from_json(const nlohmann::json& j);
    static std::expected<BenchmarkConfig, Error> from_file(const std::filesystem::path& p);
};

std::string to_string(Protocol p);
std::string to_string(SerFmt s);
std::string to_string(Compression c);
std::string to_string(QoS q);
std::string to_string(ZmqPattern z);
std::string to_string(PayloadFmt f);

Protocol    protocol_from_string(std::string_view s);
SerFmt      serfmt_from_string(std::string_view s);
Compression compression_from_string(std::string_view s);
QoS         qos_from_string(std::string_view s);
ZmqPattern  zmqpattern_from_string(std::string_view s);
PayloadFmt  payloadfmt_from_string(std::string_view s);

std::string run_folder_name(const std::string& run_id, const BenchmarkConfig& cfg);

}  // namespace pbt
