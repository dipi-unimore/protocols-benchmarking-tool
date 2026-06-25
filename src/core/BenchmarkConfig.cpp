#include "pbt/core/BenchmarkConfig.hpp"
#include "pbt/core/WireHeader.hpp"
#include <fstream>
#include <format>
#include <stdexcept>

namespace pbt {

std::expected<void, Error> BenchmarkConfig::validate() const {
    if (serializer == SerFmt::Cbor &&
        payload_format != PayloadFmt::Json &&
        payload_format != PayloadFmt::Yaml &&
        payload_format != PayloadFmt::Kv) {
        return std::unexpected(Error{
            "CBOR serializer requires json, yaml, or kv payload format"});
    }
    if (port == 0)
        return std::unexpected(Error{"port must be > 0"});
    if (duration_s == 0)
        return std::unexpected(Error{"duration must be > 0"});
    if (udp_mtu < sizeof(WireHeader) + 24 + 1)
        return std::unexpected(Error{"udp_mtu too small"});
    return {};
}

// --- string conversions ---

std::string to_string(Protocol p) {
    switch (p) {
        case Protocol::MqttTcp: return "mqtt_tcp";
        case Protocol::ZmqTcp:  return "zmq_tcp";
        case Protocol::Tcp:     return "tcp";
        case Protocol::Udp:     return "udp";
    }
    return "unknown";
}

std::string to_string(SerFmt s) {
    switch (s) {
        case SerFmt::None:     return "none";
        case SerFmt::Cbor:     return "cbor";
        case SerFmt::Protobuf: return "protobuf";
    }
    return "unknown";
}

std::string to_string(Compression c) {
    switch (c) {
        case Compression::None: return "none";
        case Compression::Zstd: return "zstd";
    }
    return "unknown";
}

std::string to_string(QoS q) {
    switch (q) {
        case QoS::AtMostOnce:  return "0";
        case QoS::AtLeastOnce: return "1";
        case QoS::ExactlyOnce: return "2";
    }
    return "0";
}

std::string to_string(ZmqPattern z) {
    switch (z) {
        case ZmqPattern::PushPull: return "push_pull";
        case ZmqPattern::PubSub:   return "pub_sub";
    }
    return "push_pull";
}

std::string to_string(PayloadFmt f) {
    switch (f) {
        case PayloadFmt::Text:   return "text";
        case PayloadFmt::Json:   return "json";
        case PayloadFmt::Yaml:   return "yaml";
        case PayloadFmt::Kv:     return "kv";
        case PayloadFmt::Binary: return "binary";
        case PayloadFmt::Random: return "random";
    }
    return "random";
}

Protocol protocol_from_string(std::string_view s) {
    if (s == "mqtt_tcp") return Protocol::MqttTcp;
    if (s == "zmq_tcp")  return Protocol::ZmqTcp;
    if (s == "tcp")      return Protocol::Tcp;
    if (s == "udp")      return Protocol::Udp;
    throw std::invalid_argument(std::format("Unknown protocol: {}", s));
}

SerFmt serfmt_from_string(std::string_view s) {
    if (s == "none")     return SerFmt::None;
    if (s == "cbor")     return SerFmt::Cbor;
    if (s == "protobuf") return SerFmt::Protobuf;
    throw std::invalid_argument(std::format("Unknown serializer: {}", s));
}

Compression compression_from_string(std::string_view s) {
    if (s == "none") return Compression::None;
    if (s == "zstd") return Compression::Zstd;
    throw std::invalid_argument(std::format("Unknown compression: {}", s));
}

QoS qos_from_string(std::string_view s) {
    if (s == "0") return QoS::AtMostOnce;
    if (s == "1") return QoS::AtLeastOnce;
    if (s == "2") return QoS::ExactlyOnce;
    throw std::invalid_argument(std::format("Unknown QoS: {}", s));
}

ZmqPattern zmqpattern_from_string(std::string_view s) {
    if (s == "push_pull") return ZmqPattern::PushPull;
    if (s == "pub_sub")   return ZmqPattern::PubSub;
    throw std::invalid_argument(std::format("Unknown zmq_pattern: {}", s));
}

PayloadFmt payloadfmt_from_string(std::string_view s) {
    if (s == "text")   return PayloadFmt::Text;
    if (s == "json")   return PayloadFmt::Json;
    if (s == "yaml")   return PayloadFmt::Yaml;
    if (s == "kv")     return PayloadFmt::Kv;
    if (s == "binary") return PayloadFmt::Binary;
    if (s == "random") return PayloadFmt::Random;
    throw std::invalid_argument(std::format("Unknown payload_format: {}", s));
}

std::string run_folder_name(const std::string& run_id, const BenchmarkConfig& cfg) {
    std::string dict = cfg.zstd_dict_path.empty() ? "" : "_dict";
    return std::format("{}_{}_{}_{}{}", run_id,
                       to_string(cfg.protocol),
                       to_string(cfg.serializer),
                       to_string(cfg.compression),
                       dict);
}

// --- JSON serialization ---

nlohmann::json BenchmarkConfig::to_json() const {
    nlohmann::json j;
    j["protocol"]                 = to_string(protocol);
    j["host"]                     = host;
    j["port"]                     = port;
    j["serializer"]               = to_string(serializer);
    j["compression"]              = to_string(compression);
    j["zstd_dict_path"]           = zstd_dict_path;
    j["payload_file"]             = payload_file;
    j["payload_format"]           = to_string(payload_format);
    j["payload_size_bytes"]       = payload_size_bytes;
    j["message_rate_hz"]          = message_rate_hz;
    j["duration_s"]               = duration_s;
    j["warmup_s"]                 = warmup_s;
    j["mqtt_topic"]               = mqtt_topic;
    j["mqtt_qos"]                 = to_string(mqtt_qos);
    j["zmq_pattern"]              = to_string(zmq_pattern);
    j["zmq_topic"]                = zmq_topic;
    j["receiver_timeout_extra_s"] = receiver_timeout_extra_s;
    j["udp_mtu"]                  = udp_mtu;
    j["metadata"]                 = metadata;
    return j;
}

std::expected<BenchmarkConfig, Error> BenchmarkConfig::from_json(const nlohmann::json& j) {
    try {
        BenchmarkConfig c;
        if (j.contains("protocol"))
            c.protocol = protocol_from_string(j["protocol"].get<std::string>());
        if (j.contains("host"))     c.host = j["host"];
        if (j.contains("port"))     c.port = j["port"];
        if (j.contains("serializer"))
            c.serializer = serfmt_from_string(j["serializer"].get<std::string>());
        if (j.contains("compression"))
            c.compression = compression_from_string(j["compression"].get<std::string>());
        if (j.contains("zstd_dict_path")) c.zstd_dict_path = j["zstd_dict_path"];
        if (j.contains("payload_file"))   c.payload_file   = j["payload_file"];
        if (j.contains("payload_format"))
            c.payload_format = payloadfmt_from_string(j["payload_format"].get<std::string>());
        if (j.contains("payload_size_bytes")) c.payload_size_bytes = j["payload_size_bytes"];
        if (j.contains("message_rate_hz"))  c.message_rate_hz  = j["message_rate_hz"];
        if (j.contains("duration_s"))       c.duration_s       = j["duration_s"];
        if (j.contains("warmup_s"))         c.warmup_s         = j["warmup_s"];
        if (j.contains("mqtt_topic"))       c.mqtt_topic       = j["mqtt_topic"];
        if (j.contains("mqtt_qos"))
            c.mqtt_qos = qos_from_string(j["mqtt_qos"].get<std::string>());
        if (j.contains("zmq_pattern"))
            c.zmq_pattern = zmqpattern_from_string(j["zmq_pattern"].get<std::string>());
        if (j.contains("zmq_topic"))              c.zmq_topic              = j["zmq_topic"];
        if (j.contains("receiver_timeout_extra_s"))
            c.receiver_timeout_extra_s = j["receiver_timeout_extra_s"];
        if (j.contains("udp_mtu"))  c.udp_mtu  = j["udp_mtu"];
        if (j.contains("metadata"))
            c.metadata = j["metadata"].get<std::map<std::string, std::string>>();
        return c;
    } catch (const std::exception& e) {
        return std::unexpected(Error{e.what()});
    }
}

std::expected<BenchmarkConfig, Error>
BenchmarkConfig::from_file(const std::filesystem::path& p) {
    std::ifstream f(p);
    if (!f)
        return std::unexpected(Error{std::format("Cannot open config file: {}", p.string())});
    try {
        return from_json(nlohmann::json::parse(f));
    } catch (const std::exception& e) {
        return std::unexpected(Error{e.what()});
    }
}

}  // namespace pbt
