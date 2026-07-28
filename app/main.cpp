#include "pbt/core/BenchmarkConfig.hpp"
#include "pbt/core/RunResult.hpp"
#include "pbt/core/BoundedBlockingQueue.hpp"
#include "pbt/core/InboundPacket.hpp"
#include "pbt/payload/PayloadSource.hpp"
#include "pbt/serialization/Serializer.hpp"
#include "pbt/compression/Compressor.hpp"
#include "pbt/transport/Sender.hpp"
#include "pbt/transport/Receiver.hpp"
#include "pbt/transport/UdpReceiver.hpp"
#include "pbt/sync/NtpSync.hpp"
#include "pbt/metrics/PacketProcessor.hpp"
#include "pbt/data/DataManager.hpp"
#include "pbt/data/CsvPacketWriter.hpp"
#include "pbt/data/CsvSenderWriter.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstdlib>
#include <format>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace pbt;
using namespace std::chrono_literals;

static void die(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << '\n';
    std::exit(1);
}

// True if `host` resolves to a loopback address (127.0.0.0/8 or ::1). Resolution-based (not a
// string match on "localhost") so it also catches /etc/hosts aliases pointing at loopback.
// Used to auto-skip NTP sync for same-host sender/receiver runs: they share one physical clock,
// so the true offset is 0 and any correction only reintroduces independent-query sync noise
// (see docs/ntp-sync.md).
static bool is_loopback_host(const std::string& host) {
    addrinfo hints{}, *res{};
    hints.ai_family = AF_UNSPEC;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0) return false;
    bool loopback = false;
    for (auto* p = res; p; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            auto addr = ntohl(reinterpret_cast<sockaddr_in*>(p->ai_addr)->sin_addr.s_addr);
            if ((addr >> 24) == 127) { loopback = true; break; }
        } else if (p->ai_family == AF_INET6) {
            if (IN6_IS_ADDR_LOOPBACK(&reinterpret_cast<sockaddr_in6*>(p->ai_addr)->sin6_addr)) {
                loopback = true; break;
            }
        }
    }
    freeaddrinfo(res);
    return loopback;
}

static void usage() {
    std::cerr << R"(pb-tool — Protocol Benchmarking Tool

Usage:
  pb-tool --mode receiver|sender [options]

Options:
  --mode receiver|sender         (required)
  --config <file>                Load config from JSON file
  --run-id <id>                  (required) Correlates sender/receiver output
  --protocol mqtt_tcp|zmq_tcp|tcp|udp
  --serializer none|cbor|protobuf
  --compression none|zstd
  --host <host>
  --port <port>
  --payload-file <file>
  --payload-format text|json|yaml|kv|binary|random
  --payload-size <bytes>         Payload size in bytes (random format only)
  --rate <hz>                    Messages per second (0 = burst)
  --duration <s>                 Measurement duration
  --warmup <s>                   Warmup duration
  --mqtt-topic <topic>
  --mqtt-qos 0|1|2
  --zmq-pattern push_pull|pub_sub
  --zmq-topic <prefix>
  --zstd-dict <path>
  --receiver-timeout-extra <s>
  --no-ntp                       Skip NTP sync
  --ntp-server <host>            NTP server to query (default: try 127.0.0.1 first,
                                 fall back to pool.ntp.org; env PBT_NTP_SERVER)
  --ntp-skip-loopback            Sender: skip NTP when --host is loopback (same clock
                                 as receiver, correction unneeded). Off by default.
  --log-sender                   Write sender_log.csv
  --meta key=value               Add metadata (repeatable)
)";
    std::exit(1);
}

// Parse key=value metadata pair
static std::pair<std::string, std::string> parse_kv(const std::string& s) {
    auto pos = s.find('=');
    if (pos == std::string::npos) die("--meta requires key=value format");
    return {s.substr(0, pos), s.substr(pos + 1)};
}

int main(int argc, char* argv[]) {
    if (argc < 2) usage();

    std::string mode;
    std::string run_id;
    std::string config_file;
    bool no_ntp     = false;
    bool log_sender = false;
    bool has_ntp_server = false;
    bool ntp_skip_loopback = false;
    std::string ntp_server_cli;
    BenchmarkConfig cfg;

    // CLI overrides
    bool has_protocol    = false;
    bool has_host        = false;
    bool has_port        = false;
    bool has_serializer  = false;
    bool has_compression = false;
    bool has_payload_file = false;
    bool has_payload_fmt  = false;
    bool has_payload_size = false;
    bool has_rate        = false;
    bool has_duration    = false;
    bool has_warmup      = false;
    bool has_mqtt_topic  = false;
    bool has_mqtt_qos    = false;
    bool has_zmq_pattern = false;
    bool has_zmq_topic   = false;
    bool has_zstd_dict   = false;
    bool has_timeout     = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) die(std::format("Missing value for {}", arg));
            return argv[++i];
        };
        if (arg == "--mode")                    { mode            = next(); }
        else if (arg == "--run-id")             { run_id          = next(); }
        else if (arg == "--config")             { config_file     = next(); }
        else if (arg == "--no-ntp")             { no_ntp          = true;  }
        else if (arg == "--ntp-server")         { has_ntp_server  = true; ntp_server_cli = next(); }
        else if (arg == "--ntp-skip-loopback")  { ntp_skip_loopback = true; }
        else if (arg == "--log-sender")         { log_sender      = true;  }
        else if (arg == "--protocol")           { has_protocol    = true; cfg.protocol    = protocol_from_string(next()); }
        else if (arg == "--host")               { has_host        = true; cfg.host        = next(); }
        else if (arg == "--port")               { has_port        = true; cfg.port        = static_cast<uint16_t>(std::stoi(next())); }
        else if (arg == "--serializer")         { has_serializer  = true; cfg.serializer  = serfmt_from_string(next()); }
        else if (arg == "--compression")        { has_compression = true; cfg.compression = compression_from_string(next()); }
        else if (arg == "--payload-file")       { has_payload_file = true; cfg.payload_file = next(); }
        else if (arg == "--payload-format")     { has_payload_fmt  = true; cfg.payload_format = payloadfmt_from_string(next()); }
        else if (arg == "--payload-size")       { has_payload_size = true; cfg.payload_size_bytes = static_cast<uint32_t>(std::stoi(next())); }
        else if (arg == "--rate")               { has_rate        = true; cfg.message_rate_hz = static_cast<uint32_t>(std::stoi(next())); }
        else if (arg == "--duration")           { has_duration    = true; cfg.duration_s  = static_cast<uint32_t>(std::stoi(next())); }
        else if (arg == "--warmup")             { has_warmup      = true; cfg.warmup_s    = static_cast<uint32_t>(std::stoi(next())); }
        else if (arg == "--mqtt-topic")         { has_mqtt_topic  = true; cfg.mqtt_topic  = next(); }
        else if (arg == "--mqtt-qos")           { has_mqtt_qos    = true; cfg.mqtt_qos    = qos_from_string(next()); }
        else if (arg == "--zmq-pattern")        { has_zmq_pattern = true; cfg.zmq_pattern = zmqpattern_from_string(next()); }
        else if (arg == "--zmq-topic")          { has_zmq_topic   = true; cfg.zmq_topic   = next(); }
        else if (arg == "--zstd-dict")          { has_zstd_dict   = true; cfg.zstd_dict_path = next(); }
        else if (arg == "--receiver-timeout-extra") { has_timeout = true; cfg.receiver_timeout_extra_s = static_cast<uint32_t>(std::stoi(next())); }
        else if (arg == "--meta")               { auto [k,v] = parse_kv(next()); cfg.metadata[k] = v; }
        else if (arg == "--help" || arg == "-h") { usage(); }
        else { die(std::format("Unknown argument: {}", arg)); }
    }

    if (mode.empty())   die("--mode is required");
    if (run_id.empty()) die("--run-id is required");

    // Load config file first, then apply CLI overrides
    if (!config_file.empty()) {
        auto res = BenchmarkConfig::from_file(config_file);
        if (!res) die(res.error().message);
        BenchmarkConfig file_cfg = std::move(*res);
        // Apply CLI overrides on top of file config
        if (has_protocol)    file_cfg.protocol    = cfg.protocol;
        if (has_host)        file_cfg.host         = cfg.host;
        if (has_port)        file_cfg.port         = cfg.port;
        if (has_serializer)  file_cfg.serializer   = cfg.serializer;
        if (has_compression) file_cfg.compression  = cfg.compression;
        if (has_payload_file) file_cfg.payload_file = cfg.payload_file;
        if (has_payload_fmt)  file_cfg.payload_format = cfg.payload_format;
        if (has_payload_size) file_cfg.payload_size_bytes = cfg.payload_size_bytes;
        if (has_rate)        file_cfg.message_rate_hz  = cfg.message_rate_hz;
        if (has_duration)    file_cfg.duration_s   = cfg.duration_s;
        if (has_warmup)      file_cfg.warmup_s     = cfg.warmup_s;
        if (has_mqtt_topic)  file_cfg.mqtt_topic   = cfg.mqtt_topic;
        if (has_mqtt_qos)    file_cfg.mqtt_qos     = cfg.mqtt_qos;
        if (has_zmq_pattern) file_cfg.zmq_pattern  = cfg.zmq_pattern;
        if (has_zmq_topic)   file_cfg.zmq_topic    = cfg.zmq_topic;
        if (has_zstd_dict)   file_cfg.zstd_dict_path = cfg.zstd_dict_path;
        if (has_timeout)     file_cfg.receiver_timeout_extra_s = cfg.receiver_timeout_extra_s;
        for (auto& [k, v] : cfg.metadata) file_cfg.metadata[k] = v;
        cfg = std::move(file_cfg);
    }

    // Validate
    if (auto v = cfg.validate(); !v) die(v.error().message);

    // NTP sync. Same-host sender+receiver share one physical clock (true offset = 0), so with
    // --ntp-skip-loopback a sender targeting a loopback address skips NTP entirely rather than
    // let two independent SNTP queries inject sync noise that can dominate — even invert the
    // sign of — a loopback transit delay that's only tens to hundreds of microseconds (opt-in,
    // off by default; see docs/ntp-sync.md).
    int64_t ntp_offset_ns      = 0;
    int64_t ntp_uncertainty_ns = 0;
    std::string ntp_server_used;
    const bool sender_loopback = ntp_skip_loopback
                               && mode == "sender" && is_loopback_host(cfg.host);

    if (no_ntp) {
        // ntp_offset_ns stays 0 — WireHeader::is_ntp_disabled() picks this up automatically.
    } else if (const char* offset_env = std::getenv("PBT_NTP_OFFSET_NS")) {
        ntp_offset_ns = std::stoll(offset_env);  // manual override, asserted accurate
    } else if (sender_loopback) {
        std::cerr << std::format(
            "[NTP] loopback target detected (--host {}) — same clock as receiver, "
            "skipping NTP sync\n", cfg.host);
    } else {
        constexpr int kSamples = 8;
        const bool explicit_server = has_ntp_server || std::getenv("PBT_NTP_SERVER");
        std::string server = has_ntp_server ? ntp_server_cli
                            : std::getenv("PBT_NTP_SERVER") ? std::getenv("PBT_NTP_SERVER")
                            : std::string{};

        std::expected<NtpInfo, Error> res;
        if (explicit_server) {
            std::cerr << std::format("[NTP] querying {} ({} samples)...\n", server, kSamples);
            res = NtpSync::query(server, kSamples);
        } else {
            // Local-first: a same-LAN/localhost time server has a far more symmetric network
            // path than a public server, so its offset estimate is proportionally far more
            // accurate — worth a cheap, short-timeout probe before falling back.
            server = "127.0.0.1";
            std::cerr << "[NTP] probing local NTP server (127.0.0.1)...\n";
            res = NtpSync::query(server, /*samples=*/2, std::chrono::milliseconds{300});
            if (!res) {
                server = "pool.ntp.org";
                std::cerr << std::format(
                    "[NTP] no local server responding, falling back to {} ({} samples)...\n",
                    server, kSamples);
                res = NtpSync::query(server, kSamples);
            }
        }
        if (!res) die(std::format("NTP failed: {}\nUse --no-ntp to skip.", res.error().message));
        ntp_offset_ns      = res->offset_ns;
        ntp_uncertainty_ns = res->uncertainty_ns;
        ntp_server_used    = server;
        std::cerr << std::format(
            "[NTP] server={} offset={:.3f} ms uncertainty=+/-{:.1f} us\n",
            server, static_cast<double>(ntp_offset_ns) / 1e6,
            static_cast<double>(ntp_uncertainty_ns) / 1000.0);
    }
    const NtpInfo ntp_info{ntp_offset_ns, ntp_uncertainty_ns};

    DataManager dm = DataManager::from_env(run_id, cfg);
    if (auto r = dm.init(run_id, cfg); !r) die(r.error().message);

    // ---- SENDER MODE ----
    if (mode == "sender") {
        auto src_res = PayloadSource::create(cfg);
        if (!src_res) die(src_res.error().message);
        auto ser = Serializer::create(cfg.serializer, to_string(cfg.payload_format));
        auto cmp = Compressor::create(cfg.compression, cfg.zstd_dict_path);
        auto sender = Sender::create(cfg, std::move(*src_res), std::move(ser),
                                     std::move(cmp), ntp_info);

        if (auto r = sender->connect(); !r)
            die("connect: " + r.error().message);

        std::unique_ptr<CsvSenderWriter> sw;
        if (log_sender) {
            sw = std::make_unique<CsvSenderWriter>(dm.output_dir() / "sender_log.csv");
        }

        const uint64_t warmup_msgs = static_cast<uint64_t>(cfg.warmup_s)
                                   * cfg.message_rate_hz;
        const uint64_t measure_msgs = static_cast<uint64_t>(cfg.duration_s)
                                    * cfg.message_rate_hz;
        const bool burst = (cfg.message_rate_hz == 0);

        auto interval_ns = burst ? std::chrono::nanoseconds{0}
                                 : std::chrono::nanoseconds{
                                       1'000'000'000LL / cfg.message_rate_hz};

        uint64_t seq_id = 0;
        bool rate_warned = false;
        auto last_warn_time = std::chrono::steady_clock::now();

        // Warmup
        std::cerr << std::format("[SENDER] warmup: {} messages\n", warmup_msgs);
        auto t_start = std::chrono::steady_clock::now();
        for (uint64_t i = 0; i < warmup_msgs; ++i, ++seq_id) {
            auto t_target = t_start + interval_ns * static_cast<int64_t>(i);
            auto res = sender->send(seq_id, /*is_warmup=*/true);
            if (!res) { std::cerr << "[WARN] send: " << res.error().message << '\n'; }
            if (sw && res) sw->write_row(*res, res->original_size,
                                          static_cast<uint32_t>(sizeof(WireHeader))
                                          + res->payload_size);
            if (!burst) {
                auto now = std::chrono::steady_clock::now();
                if (now < t_target) std::this_thread::sleep_until(t_target);
                else if (now - t_target > interval_ns * 2) {
                    if (!rate_warned || now - last_warn_time > 1s) {
                        double elapsed_s = std::chrono::duration<double>(now - t_start).count();
                        double measured_hz = elapsed_s > 0.0 ? (i + 1) / elapsed_s : 0.0;
                        std::cerr << std::format("[WARN] rate behind target: expected={} hz measured={:.0f} hz\n",
                                                 cfg.message_rate_hz, measured_hz);
                        rate_warned = true;
                        last_warn_time = now;
                    }
                }
            }
        }

        // Measurement
        std::cerr << std::format("[SENDER] measuring: {} messages\n", measure_msgs);
        t_start = std::chrono::steady_clock::now();
        for (uint64_t i = 0; i < measure_msgs; ++i, ++seq_id) {
            auto t_target = t_start + interval_ns * static_cast<int64_t>(i);
            auto res = sender->send(seq_id, /*is_warmup=*/false);
            if (!res) { std::cerr << "[WARN] send: " << res.error().message << '\n'; }
            if (sw && res) sw->write_row(*res, res->original_size,
                                          static_cast<uint32_t>(sizeof(WireHeader))
                                          + res->payload_size);
            if (!burst) {
                auto now = std::chrono::steady_clock::now();
                if (now < t_target) std::this_thread::sleep_until(t_target);
                else if (now - t_target > interval_ns * 2) {
                    if (!rate_warned || now - last_warn_time > 1s) {
                        double elapsed_s = std::chrono::duration<double>(now - t_start).count();
                        double measured_hz = elapsed_s > 0.0 ? (i + 1) / elapsed_s : 0.0;
                        std::cerr << std::format("[WARN] rate behind target: expected={} hz measured={:.0f} hz\n",
                                                 cfg.message_rate_hz, measured_hz);
                        rate_warned = true;
                        last_warn_time = now;
                    }
                }
            }
        }

        // Sentinel
        if (auto r = sender->send_sentinel(seq_id - 1); !r)
            std::cerr << "[WARN] sentinel: " << r.error().message << '\n';

        sender->disconnect();
        if (sw) sw->flush();
        std::cerr << "[SENDER] done. msgs_sent=" << measure_msgs << '\n';
        return 0;
    }

    // ---- RECEIVER MODE ----
    if (mode == "receiver") {
        BoundedBlockingQueue<InboundPacket> queue(65536);
        auto ser = Serializer::create(cfg.serializer, to_string(cfg.payload_format));
        auto cmp = Compressor::create(cfg.compression, cfg.zstd_dict_path);

        CsvRowContext ctx{run_id,
                          to_string(cfg.protocol),
                          to_string(cfg.serializer),
                          to_string(cfg.compression),
                          cfg.zstd_dict_path};
        CsvPacketWriter csv_writer(dm.output_dir() / "packets.csv", ctx);
        PacketProcessor processor(queue, *ser, *cmp, csv_writer, cfg);
        processor.start();

        auto receiver = Receiver::create(cfg, queue, ntp_info);
        if (auto r = receiver->bind(); !r)
            die("bind: " + r.error().message);
        if (auto r = receiver->start(); !r)
            die("start: " + r.error().message);

        std::cout << std::format("[READY] listening on {}://0.0.0.0:{}\n",
                                  to_string(cfg.protocol), cfg.port)
                  << std::flush;

        // Wait for sentinel or timeout
        auto deadline = std::chrono::steady_clock::now()
                      + std::chrono::seconds{cfg.duration_s + cfg.warmup_s
                                             + cfg.receiver_timeout_extra_s};
        while (!receiver->sentinel_received() &&
               std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(100ms);
        }
        if (!receiver->sentinel_received())
            std::cerr << "[WARN] timeout — sentinel not received\n";

        receiver->stop();
        queue.stop();
        processor.stop();

        std::optional<uint64_t> msgs_sent;
        if (receiver->sentinel_received())
            msgs_sent = receiver->msgs_sent_from_sentinel();

        processor.set_overflow_count(receiver->overflow_count());
        if (cfg.protocol == Protocol::Udp) {
            auto* udp = dynamic_cast<UdpReceiver*>(receiver.get());
            if (udp) processor.set_fragment_timeout_count(udp->fragment_timeout_count());
        }
        processor.set_msgs_sent(msgs_sent);

        csv_writer.flush();
        RunResult result = processor.finalize();
        result.ntp_server = ntp_server_used;
        if (auto r = dm.save_result(result); !r)
            std::cerr << "[WARN] save_result: " << r.error().message << '\n';

        // Print summary
        std::cout << std::format("[DONE] received={} loss_pct={:.2f} e2e_mean_us={:.1f}\n",
                                  result.msgs_received,
                                  result.packet_loss_pct,
                                  result.e2e.mean_us);
        return 0;
    }

    die(std::format("Unknown --mode: {}. Must be 'sender' or 'receiver'.", mode));
}
