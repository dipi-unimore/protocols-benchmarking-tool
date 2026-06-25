# pb-tool — Protocol Benchmarking Tool

Measures end-to-end performance of IoT/mobility communication protocols. Designed for cross-machine benchmarking with separate Sender and Receiver processes correlated by a shared `--run-id`.

## Protocols, Serializers, Compressors

| Transport | Serializer | Compressor |
|-----------|-----------|------------|
| Raw TCP   | None (passthrough) | None |
| Raw UDP (+ app-level fragmentation) | CBOR (JSON/YAML/KV payloads only) | Zstd (standard) |
| ZeroMQ TCP (PUSH/PULL or PUB/SUB) | Protobuf | Zstd + pre-trained dictionary |
| MQTT v3.1.1 over TCP | | |

## Prerequisites

- CMake ≥ 3.28
- C++23-capable compiler (GCC ≥ 13, Clang ≥ 16, or Apple Clang ≥ 15)
- Internet access (FetchContent downloads all dependencies at configure time)

All dependencies are fetched automatically: nlohmann_json, yaml-cpp, spdlog, GoogleTest, protobuf, zstd, libzmq, cppzmq, paho-mqtt-cpp (with bundled paho-mqtt-c), tinycbor.

## Build

```bash
# macOS (Apple Silicon or Intel)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.logicalcpu)

# Linux
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# ARM cross-compile (from x86_64 Linux)
cmake -B build-arm -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=cmake/arm-toolchain.cmake
cmake --build build-arm -j$(nproc)

# Run tests
ctest --test-dir build --output-on-failure
# MQTT integration tests require: export MQTT_HOST=... MQTT_PORT=...
```

---

## Getting Started — Simple Examples

All examples below run on a single machine (loopback). For cross-machine runs substitute the receiver's IP for `127.0.0.1` in `--host`.

### Example 1 — Raw TCP, no serialization, no compression

The simplest possible run. Sends 100 random-bytes messages at 500 msg/s over TCP.

**Terminal 1 (Receiver):**
```bash
./build/app/pb-tool \
  --mode receiver \
  --protocol tcp \
  --port 5555 \
  --run-id test01
```

Wait for the `[READY]` line, then:

**Terminal 2 (Sender):**
```bash
./build/app/pb-tool \
  --mode sender \
  --protocol tcp \
  --host 127.0.0.1 --port 5555 \
  --payload-format random --payload-size 512 \
  --rate 500 --duration 10 \
  --run-id test01
```

Results in `./results/test01_tcp_none_none/`:
- `packets.csv` — per-packet timestamps and derived latencies
- `summary.json` — mean/p50/p95/p99/p99.9 for e2e delay and each pipeline stage

---

### Example 2 — Raw UDP

Replace `--protocol tcp` with `--protocol udp`. Application-level fragmentation is handled automatically for payloads larger than MTU (default 1472 B).

```bash
# Receiver
./build/app/pb-tool --mode receiver --protocol udp --port 5556 --run-id test02

# Sender
./build/app/pb-tool --mode sender --protocol udp \
  --host 127.0.0.1 --port 5556 \
  --payload-format random --payload-size 2048 \
  --rate 200 --duration 10 --run-id test02
```

---

### Example 3 — ZeroMQ PUSH/PULL with Protobuf + Zstd

```bash
# Receiver (PULL binds)
./build/app/pb-tool \
  --mode receiver \
  --protocol zmq_tcp --zmq-pattern push_pull \
  --port 5557 \
  --run-id test03

# Sender (PUSH connects)
./build/app/pb-tool \
  --mode sender \
  --protocol zmq_tcp --zmq-pattern push_pull \
  --host 127.0.0.1 --port 5557 \
  --serializer protobuf --compression zstd \
  --payload-format random --payload-size 1024 \
  --rate 1000 --duration 10 \
  --run-id test03
```

---

### Example 4 — ZeroMQ PUB/SUB

> **Topology:** Publisher **binds**, Subscriber **connects**. Start the Sender (Publisher) first — messages published before the subscription is established are dropped.

```bash
# Sender/Publisher (start first — PUB binds)
./build/app/pb-tool \
  --mode sender \
  --protocol zmq_tcp --zmq-pattern pub_sub \
  --port 5558 \
  --payload-format random --payload-size 256 \
  --rate 100 --duration 10 \
  --run-id test04

# Receiver/Subscriber (SUB connects)
./build/app/pb-tool \
  --mode receiver \
  --protocol zmq_tcp --zmq-pattern pub_sub \
  --host 127.0.0.1 --port 5558 \
  --run-id test04
```

---

### Example 5 — MQTT QoS 1 with CBOR and a JSON payload file

Requires a running MQTT broker (e.g. `mosquitto -p 1883`).

```bash
# Receiver
./build/app/pb-tool \
  --mode receiver \
  --protocol mqtt_tcp \
  --host 127.0.0.1 --port 1883 \
  --mqtt-topic pbt/bench --mqtt-qos 1 \
  --run-id test05

# Sender
./build/app/pb-tool \
  --mode sender \
  --protocol mqtt_tcp \
  --host 127.0.0.1 --port 1883 \
  --mqtt-topic pbt/bench --mqtt-qos 1 \
  --serializer cbor --compression none \
  --payload-file payloads/sample_telemetry.json --payload-format json \
  --rate 50 --duration 10 \
  --run-id test05
```

---

### Example 6 — Using a config file (recommended for repeated runs)

Config files in `configs/examples/` bundle all parameters. CLI flags override individual fields.

```bash
# Receiver
./build/app/pb-tool --mode receiver \
  --config configs/examples/tcp_protobuf_zstd.json \
  --run-id run001

# Sender (override host via CLI)
./build/app/pb-tool --mode sender \
  --config configs/examples/tcp_protobuf_zstd.json \
  --host 192.168.1.42 \
  --run-id run001
```

---

### Example 7 — Warmup + metadata labels

The warmup period excludes initial packets from statistics (absorbs JIT, buffer fill, connection setup). Use `--meta` to tag runs for later filtering.

```bash
./build/app/pb-tool --mode sender \
  --protocol tcp --host 127.0.0.1 --port 5555 \
  --rate 1000 --duration 30 --warmup 5 \
  --meta env=lab --meta distance_m=10 --meta obstacle=none \
  --run-id exp01
```

---

### Reading Results

```bash
# Quick summary
cat results/test01_tcp_none_none/summary.json | python3 -m json.tool

# Plot e2e delay over time (requires pandas + matplotlib)
python3 - <<'EOF'
import pandas as pd, matplotlib.pyplot as plt
df = pd.read_csv("results/test01_tcp_none_none/packets.csv")
df["e2e_delay_us"].plot()
plt.xlabel("packet"); plt.ylabel("e2e delay (µs)")
plt.savefig("latency.png")
EOF
```

---

## Full CLI Reference

```
pb-tool [OPTIONS]

Required:
  --mode receiver|sender
  --run-id <id>            Short alphanumeric identifier for this run

Configuration:
  --config <file>          Load Run Config JSON (CLI flags override file fields)
  --protocol mqtt_tcp|zmq_tcp|tcp|udp
  --serializer none|cbor|protobuf
  --compression none|zstd
  --host <host>            Sender target host (Receiver ignores this, binds 0.0.0.0)
  --port <port>
  --payload-file <path>    File in payloads/; omit for random bytes
  --payload-format text|json|yaml|kv|binary|random

Timing:
  --rate <hz>              Target messages/second (0 = burst mode)
  --duration <s>           Measurement phase duration (default: 30)
  --warmup <s>             Warmup phase duration (default: 5)

Protocol-specific:
  --mqtt-topic <topic>
  --mqtt-qos 0|1|2
  --zmq-pattern push_pull|pub_sub
  --zmq-topic <prefix>     ZMQ PUB/SUB topic prefix (empty = subscribe-all)
  --zstd-dict <path>       Pre-trained Zstd dictionary file (both sides need it)

Receiver:
  --receiver-timeout-extra <s>   Extra seconds to wait for late packets (default: 5)
                                 Rule of thumb: 3× expected max one-way latency

Other:
  --no-ntp                 Skip NTP clock sync (use if machines share system NTP/PTP)
  --log-sender             Write sender_log.csv (Sender only)
  --meta key=value         Add metadata tag (repeatable)
```

## NTP Clock Synchronization

The tool queries an SNTP server at startup and embeds the offset in every wire header. End-to-end delay is corrected for clock differences between machines. A failed NTP query is a hard error — use `--no-ntp` to skip if machines share system NTP/PTP.

Override the default server (`pool.ntp.org`) via `PBT_NTP_SERVER`. Supply an offset directly via `PBT_NTP_OFFSET_NS` to skip the network query.

## Warmup Phase

The first `warmup_s × rate` messages are sent with the warmup flag. The Receiver processes them normally but excludes them from summary statistics and percentiles. Useful to absorb connection setup, JIT, and buffer fill.

## Receiver Timeout

When no sentinel arrives within `duration + warmup + receiver_timeout_extra` seconds, the Receiver terminates and writes `msgs_sent: null` in `summary.json`. Increase `--receiver-timeout-extra` for high-latency links.

## Zstd Dictionary

```bash
zstd --train payloads/*.json -o payloads/dict.zstd
# Copy dict to both machines, then:
pb-tool --mode sender   ... --zstd-dict payloads/dict.zstd
pb-tool --mode receiver ... --zstd-dict payloads/dict.zstd
```

## Protobuf Schema

The default schema (`proto/pbt_message.proto`) wraps raw bytes in a `PbtPayload` message. To benchmark a custom schema:

1. Replace `proto/pbt_message.proto`
2. Update `src/serialization/ProtobufSerializer.cpp`
3. Rebuild

## Environment Variables

Copy `.env.example` to `.env` and export variables in your shell before running.

| Variable | Default | Description |
|----------|---------|-------------|
| `PBT_OUTPUT_DIR` | `./results` | Base directory for run output |
| `PBT_NTP_SERVER` | `pool.ntp.org` | SNTP server hostname |
| `PBT_NTP_OFFSET_NS` | (unset) | Manual NTP offset; skips network query |
| `MQTT_HOST`, `MQTT_PORT` | localhost / 1883 | Used by integration tests |

## Output Schema

### packets.csv

One row per received message (warmup excluded):

```
run_id, protocol, serializer, compression, zstd_dict, crc_ok,
seq_id, payload_size_bytes, wire_size_bytes,
ts_created_ns, ts_serialized_ns, ts_compressed_ns, ts_sent_ns,
ts_received_ns, ts_decompressed_ns, ts_deserialized_ns, ts_processed_ns,
ntp_offset_sender_ns, ntp_offset_receiver_ns,
e2e_delay_us, serialization_us, compression_us, transport_us,
decompression_us, deserialization_us, processing_us,
jitter_us, is_out_of_order, is_duplicate, seq_gap
```

Key derived columns:
- `e2e_delay_us = (ts_received_ns − ts_sent_ns + ntp_offset_sender_ns − ntp_offset_receiver_ns) / 1000`
- `jitter_us = |e2e_delay_us − previous_packet_e2e_delay_us|`
- `seq_gap > 0` → number of packets presumed lost before this one

### summary.json

Aggregated statistics: mean, stddev, min, max, p50, p95, p99, p99.9 for e2e delay and each pipeline stage.
