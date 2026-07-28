# Running

All examples below run on a single machine (loopback). For cross-machine runs substitute the receiver's IP for `127.0.0.1` in `--host`. Always start the **Receiver first** and wait for the `[READY]` log line before launching the Sender.

---

## Example 1 — Raw TCP, no serialization, no compression

The simplest possible run. Sends 512-byte random-bytes messages at 500 msg/s over TCP for 10 seconds.

**Terminal 1 (Receiver):**
```bash
./build/app/pb-tool \
  --mode receiver \
  --protocol tcp \
  --port 5555 \
  --run-id test01
```

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

Results written to `./results/test01_tcp_none_none/`.

---

## Example 2 — Raw UDP

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

## Example 3 — ZeroMQ PUSH/PULL with Protobuf + Zstd

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

## Example 4 — ZeroMQ PUB/SUB

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

## Example 5 — MQTT QoS 1 with CBOR and a JSON payload file

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

## Example 6 — Using a Run Config file (recommended for repeated runs)

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

## Example 7 — Warmup + metadata labels

The warmup period excludes initial packets from statistics (absorbs JIT, buffer fill, connection setup). Use `--meta` to tag runs for later filtering.

```bash
./build/app/pb-tool --mode sender \
  --protocol tcp --host 127.0.0.1 --port 5555 \
  --rate 1000 --duration 30 --warmup 5 \
  --meta env=lab --meta distance_m=10 --meta obstacle=none \
  --run-id exp01
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
  --ntp-server <host>      NTP server to query (default: try 127.0.0.1 first, fall
                           back to pool.ntp.org; env PBT_NTP_SERVER)
  --ntp-skip-loopback      Sender only, opt-in: skip NTP when --host is loopback
                           (same clock as receiver — see NTP section below)
  --log-sender             Write sender_log.csv (Sender only)
  --meta key=value         Add metadata tag (repeatable)
```

---

## NTP Clock Synchronisation

The tool queries an SNTP server at startup and embeds the offset (plus an uncertainty bound) in
every `WireHeader`. End-to-end delay is corrected for clock differences between machines. A single
SNTP exchange has an error proportional to network path asymmetry (hundreds of µs to several ms
against a public server) — see [docs/ntp-sync.md](ntp-sync.md) for the full analysis of why this
matters and how it's mitigated. A failed NTP query is a hard error.

- Each side samples the server **8 times** and keeps the lowest-RTT exchange (least path
  asymmetry, standard NTP client practice), instead of trusting a single shot.
- Server resolution is **local-first**: without `--ntp-server`/`PBT_NTP_SERVER`, pb-tool probes
  `127.0.0.1` (cheap, short timeout) before falling back to `pool.ntp.org` — a local time daemon
  has a far more symmetric path, hence lower error.
- `--ntp-skip-loopback` (sender only, **off by default**) skips NTP entirely when `--host` is a
  loopback address — same physical clock as the receiver, true offset is 0, correction only adds
  noise. The decision travels to the receiver on the wire (`WireHeader` flag), so the receiver
  ignores its own offset for that run too; nothing needs to be passed to the receiver.
- `--no-ntp` (both sides) skips sync entirely; `e2e_delay_us` is then the raw, uncorrected
  `ts_received_ns - ts_sent_ns`.

| Variable | Default | Description |
|----------|---------|-------------|
| `PBT_NTP_SERVER` | (probe `127.0.0.1`, then `pool.ntp.org`) | SNTP server hostname, same as `--ntp-server` |
| `PBT_NTP_OFFSET_NS` | (unset) | Supply an offset directly; skips the network query (uncertainty reported as 0) |

---

## Warmup Phase

The first `warmup_s × rate` messages are transmitted with the warmup flag set in the `WireHeader`. The Receiver processes them normally but excludes them from summary statistics and percentiles. Absorbs connection setup, JIT, and buffer fill.

---

## Receiver Timeout

When no sentinel arrives within `duration + warmup + receiver_timeout_extra` seconds, the Receiver terminates and writes `msgs_sent: null` in `summary.json`. Increase `--receiver-timeout-extra` for high-latency links or slow senders.

---

## Zstd Dictionary

A pre-trained Zstd dictionary significantly improves compression ratio on small, repetitive IoT payloads. Both Sender and Receiver must use the same dictionary file.

```bash
# Train dictionary from representative payloads
zstd --train payloads/*.json -o payloads/dict.zstd

# Copy dict.zstd to both machines, then:
./build/app/pb-tool --mode sender   ... --zstd-dict payloads/dict.zstd
./build/app/pb-tool --mode receiver ... --zstd-dict payloads/dict.zstd
```

---

## Protobuf Custom Schema

The default schema (`proto/pbt_message.proto`) wraps raw bytes in a `PbtPayload` message. To benchmark a custom schema:

1. Replace `proto/pbt_message.proto` with your schema.
2. Update `src/serialization/ProtobufSerializer.cpp` to use your generated types.
3. Rebuild.

---

## Output Schema

### `packets.csv`

One row per received message (warmup excluded):

```
run_id, protocol, serializer, compression, zstd_dict, crc_ok,
seq_id, payload_size_bytes, wire_size_bytes,
ts_created_ns, ts_serialized_ns, ts_compressed_ns, ts_sent_ns,
ts_received_ns, ts_decompressed_ns, ts_deserialized_ns, ts_processed_ns,
ntp_offset_sender_ns, ntp_offset_receiver_ns,
ntp_uncertainty_sender_ns, ntp_uncertainty_receiver_ns, ntp_disabled,
e2e_delay_us, ntp_sync_uncertainty_us, serialization_us, compression_us, transport_us,
decompression_us, deserialization_us, processing_us,
jitter_us, is_out_of_order, is_duplicate, seq_gap
```

Key derived columns:

- `e2e_delay_us = (ts_received_ns − ts_sent_ns + ntp_offset_receiver_ns − ntp_offset_sender_ns) / 1000`, or the raw uncorrected delta if `ntp_disabled = 1` (both offsets treated as 0 in that case regardless of their stored values)
- `ntp_sync_uncertainty_us` — ± band on `e2e_delay_us` from clock-sync error (sum of both sides' uncertainty, in µs); 0 when `ntp_disabled = 1`. See [docs/ntp-sync.md](ntp-sync.md) — don't trust `e2e_delay_us` at a finer resolution than this.
- `jitter_us = |e2e_delay_us − previous_packet_e2e_delay_us|`
- `seq_gap > 0` → number of packets presumed lost before this one

### `summary.json`

Aggregated statistics: mean, stddev, min, max, p50, p95, p99, p99.9 for `e2e` delay and each pipeline stage (`serialization`, `compression`, `transport`, `decompression`, `deserialization`). Also includes: `msgs_sent`, `msgs_received`, `packet_loss_pct`, `out_of_order_count`, `duplicate_count`, `crc_error_count`, `overflow_count`, `throughput_bytes_per_sec`, `throughput_msgs_per_sec`, and an `ntp` block (`server`, `disabled`, both sides' `*_offset_ns`/`*_uncertainty_ns`, `sync_uncertainty_us`) — see [docs/ntp-sync.md](ntp-sync.md).
