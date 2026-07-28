# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**Protocol Benchmarking Tool** — C++23 benchmarking framework for IoT/mobility communication protocols. Sender and Receiver are **separate processes** run on different machines, correlated by a shared `--run-id`.

## Build

Requires CMake ≥ 3.28 and a C++23 compiler. All dependencies are fetched automatically via FetchContent.

```bash
# Configure + build (Intel/ARM macOS)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.logicalcpu)

# Configure + build (Linux)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# ARM cross-compile
cmake -B build-arm -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=cmake/arm-toolchain.cmake
cmake --build build-arm -j$(nproc)

# Tests
ctest --test-dir build --output-on-failure
# MQTT tests require: export MQTT_HOST=... MQTT_PORT=...
```

## Running

```bash
# Receiver (start first, wait for [READY])
./build/pb-tool --mode receiver --config configs/examples/tcp_none_none.json --run-id 001

# Sender
./build/pb-tool --mode sender --config configs/examples/tcp_none_none.json \
                   --run-id 001 --host <receiver-ip>
```

## Architecture

### Wire format
Fixed 80-byte `WireHeader` (see `include/pbt/core/WireHeader.hpp`) prepended to every message. Carries sequence ID, payload sizes, serializer/compressor IDs, CRC32 over compressed payload, send-side timestamps, and the sender's NTP offset + sync uncertainty. `static_assert(sizeof(WireHeader) == 80)` enforces layout.

For UDP: each datagram is prefixed with a 24-byte `FragmentHeader` (see `FragmentHeader.hpp`) enabling application-level fragmentation and reassembly.

### Pipeline
```
PayloadSource → Serializer → Compressor → Transport → network → Receiver → BoundedBlockingQueue
                                                                              ↓
                                                                      PacketProcessor (single thread)
                                                                              ↓
                                                                       CsvPacketWriter
```

### Key abstractions

| Abstraction | Interface | Implementations |
|---|---|---|
| Transport | `Sender` / `Receiver` | MqttTcp, ZmqTcp, Tcp, Udp |
| Serialization | `Serializer` | None, CBOR, Protobuf |
| Compression | `Compressor` | Noop, Zstd |
| Payload | `PayloadSource` | Random, PlainText, Json, Yaml, Kv, Binary |
| Metrics | `PacketProcessor` | Single processing thread; WelfordAccumulator + reservoir sampling |
| Persistence | `DataManager` | Writes config.json, packets.csv, summary.json |

### Receiver concurrency model
Network thread: stamps `ts_received`, parses `WireHeader`, `try_push` to `BoundedBlockingQueue` (non-blocking; drops on full). Single processing thread: blocking `pop`, decompress, deserialize, compute metrics, write CSV row. No mutex on sequential state → deterministic metric computation.

### Clock sync
SNTP (RFC 4330) via stdlib UDP sockets. Sender embeds its NTP offset in every `WireHeader.ntp_offset_ns`. Offset convention: `true_time = local_reading + offset`. E2E delay corrected: `(ts_received - ts_sent + receiver_offset - sender_offset) / 1000.0 µs`. Hard error on NTP failure; bypass with `--no-ntp` (must be passed to both sender and receiver).

A single SNTP exchange has an offset error proportional to network path asymmetry — hundreds of µs to several ms against a public server — which can dominate the transport delay itself on loopback/LAN runs (see `docs/ntp-sync.md`). Mitigations, all in `NtpSync::query` / `app/main.cpp` unless noted:
- **Multi-sample + min-RTT filter**: `NtpSync::query` takes `samples` (default 8) independent exchanges and keeps the one with the lowest RTT (`(t4-t1)-(t3-t2)`), the sample statistically least affected by path asymmetry. Returns `NtpInfo{offset_ns, uncertainty_ns}` (`uncertainty_ns` = best-sample RTT/2, an error bound).
- **Loopback skip (opt-in, `--ntp-skip-loopback`, off by default)**: a sender whose `--host` resolves to a loopback address (`is_loopback_host()` in `app/main.cpp`) skips NTP entirely — same physical clock as the receiver, true offset is 0, correction only adds noise. The resulting `WireHeader.ntp_disabled` flag (bit 3 of `flags`) forces the *receiver* to also ignore its own (possibly nonzero, independently-queried) offset for that run in `compute_e2e_delay_us` — no coordination between the two processes required beyond passing the flag to the sender.
- **`--ntp-server <host>` / `PBT_NTP_SERVER`**: explicit server override. Without one, resolution is local-first: probe `127.0.0.1` (2 samples, 300 ms timeout — a local time daemon has a far more symmetric path, hence lower error), fall back to `pool.ntp.org` (full sample count) if nothing answers.
- **Uncertainty reporting**: each side's `uncertainty_ns` rides the wire/local packet alongside the offset; `PacketProcessor` captures both from the first packet and exposes `ntp_sync_uncertainty_us` (sum of both sides' uncertainty, in µs) per-row in `packets.csv` and as `summary.json["ntp"]`, so a run's `e2e_delay_us` can be judged against the sync error that produced it instead of trusted blindly.

### CMake targets
- `pbt_core`, `pbt_payload`, `pbt_serialization`, `pbt_compression` — no network deps
- `pbt_transport` — links ZMQ (cppzmq + libzmq-static), paho-mqtt-cpp
- `pbt_sync` — NTP (stdlib sockets only)
- `pbt_metrics` — WelfordAccumulator, reservoir sampling, PacketProcessor
- `pbt_data` — CSV writers, DataManager
- `pbt_proto` — generated protobuf sources from `proto/pbt_message.proto`

## DataManager

All persistence flows through `DataManager`. Output root configurable via `PBT_OUTPUT_DIR` env var (default `./results`). Run folder named `{run_id}_{protocol}_{serializer}_{compression}`.

## Security

Secrets (broker credentials) in `.env` only. `.env` in `.gitignore`. Never log values that embed credentials. Use `MQTT_USER` / `MQTT_PASSWORD` env vars for broker auth.

## Optimization Standard

All code must be fully optimized: minimal algorithmic complexity, parallelism/vectorization where applicable, DRY, zero dead code. Second pass if uncertain.
