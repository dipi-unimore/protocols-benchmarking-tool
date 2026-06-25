# Protocols Benchmarking Tool

Ubiquitous language for the benchmarking framework that measures communication protocol performance in IoT/IIoT and Intelligent Mobility scenarios.

## Language

### Execution

**Run**:
A single timed execution of a specific configuration, consisting of a warmup phase followed by a measurement phase. A Run always has exactly one Sender and one Receiver, potentially on different machines.
_Avoid_: trial, experiment, test, benchmark session

**Run Config**:
A JSON file fully describing a single Run's Configuration. Passed to both Sender and Receiver via `--config <file>`. CLI flags override individual fields when specified alongside `--config`. The same file is copied to both machines to guarantee identical configuration and serves as the reproducibility record for the Run.
_Avoid_: settings file, config file, parameters file

**Run ID**:
A short alphanumeric identifier chosen by the researcher and passed identically to both Sender and Receiver (`--run-id`). Used to correlate output files from the same Run across machines.
_Avoid_: session_id, campaign_id

**Configuration**:
The complete set of parameters that define a Run, carried in a Run Config file and/or CLI flags:

| Field | Description |
|---|---|
| `protocol` | `mqtt_tcp` \| `zmq_tcp` \| `tcp` \| `udp` |
| `serializer` | `none` \| `cbor` \| `protobuf` |
| `compression` | `none` \| `zstd` |
| `zstd_dict` | optional path to pre-trained Zstd dictionary file |
| `payload_file` | path to file in `payloads/`; omit for random bytes |
| `payload_format` | `text` \| `json` \| `yaml` \| `kv` \| `binary` |
| `payload_size` | bytes per Message (used when `payload_file` is absent) |
| `message_rate` | target messages per second (0 = burst) |
| `duration` | measurement phase duration in seconds |
| `warmup_duration` | warmup phase duration in seconds |
| `host` | Sender target host (Receiver binds to `0.0.0.0`) |
| `port` | transport port |
| `mqtt_topic` | MQTT topic string (MQTT only) |
| `mqtt_qos` | 0 \| 1 \| 2 (MQTT only) |
| `zmq_pattern` | `push_pull` \| `pub_sub` (ZMQ only) |
| `zmq_topic` | ZMQ PUB/SUB prefix, default `""` (ZMQ PUB/SUB only) |
| `receiver_timeout_extra` | seconds to wait after `duration` before forcing termination (default 5) |
| `udp_mtu` | UDP MTU for fragmentation, default 1472 (UDP only) |
| `metadata` | free-form key-value pairs (string→string) annotating the physical/environmental context of the Run (e.g., `{"env": "lab", "distance_m": "50"}`). Written to `config.json` and `summary.json` only — not to `packets.csv`. |

_Avoid_: settings, options, params

### Roles

**Sender**:
The process that generates, serializes, compresses, and transmits messages at a configured rate. Stamps send-side timestamps into every message header before transmission.
_Avoid_: producer, publisher, client

**Receiver**:
The process that receives messages, stamps the receive timestamp immediately, and enqueues them for processing. Computes and writes per-packet metrics to CSV during the Run.
_Avoid_: consumer, subscriber, server

### Protocol Parameters

**QoS Level**:
The MQTT Quality of Service level for a Run: 0 (at-most-once, no confirmation), 1 (at-least-once, possible duplicates), or 2 (exactly-once, four-way handshake). Applies only to MQTT runs; ignored for all other protocols.
_Avoid_: quality level, delivery guarantee

**MQTT Topic**:
The MQTT topic string on which Messages are published and subscribed during a Run (e.g., `vehicle/telemetry`). Configurable to reflect real deployment topic hierarchies. Applies only to MQTT runs.
_Avoid_: channel, subject, queue

**ZMQ Topic**:
The message prefix used for ZMQ PUB/SUB filtering (e.g., `pbt.bench`). The Sender prepends this prefix to each Message frame; the Receiver subscribes with this prefix. Empty string means subscribe-all. Not applicable to PUSH/PULL runs.
_Avoid_: ZMQ channel, ZMQ subject

**Zstd Dictionary**:
An optional pre-trained Zstd compression dictionary built from representative payload samples (`zstd --train`). When provided via `--zstd-dict`, both Sender and Receiver load the same file. The dictionary ID is embedded in the Zstd frame and verified automatically on decompression. Without `--zstd-dict`, standard Zstd is used. The two modes are distinct, comparable benchmark configurations.
_Avoid_: compression dictionary, shared dictionary

**ZMQ Pattern**:
The ZeroMQ socket pattern used for a Run. `push_pull` provides back-pressure on the Sender when ZMQ's internal buffer reaches its high-water mark. `pub_sub` drops messages silently on the Sender side when the buffer fills. Both are valid benchmark axes with different congestion semantics.
_Avoid_: ZMQ mode, ZMQ type

### Messaging

**Message**:
The unit of transmission in a Run. Consists of a fixed WireHeader (metadata + send-side timestamps) followed by the serialized and compressed payload bytes.
_Avoid_: packet, frame, event

**WireHeader**:
A fixed 72-byte binary structure prepended to every Message. Carries sequence ID, payload sizes, serializer/compressor identifiers, a CRC32 integrity field over the payload bytes, send-side timestamps, and the Sender's NTP clock offset. All fields are naturally aligned; no compiler padding is added.
_Avoid_: header, metadata header

**Integrity Check**:
A CRC32 checksum computed by the Sender over the compressed payload bytes and stored in the WireHeader. Validated by the Receiver immediately after receiving the full Message and before decompression. Applied uniformly to all protocols, not only UDP. CRC failures increment `crc_error_count` in RunResult without aborting the Run.
_Avoid_: checksum, hash, digest

**Payload**:
The application data bytes carried inside a Message, after serialization and compression. Sourced from a file in the `payloads/` directory or generated randomly.
_Avoid_: body, content, data

**Sender Log**:
An optional CSV written by the Sender (enabled with `--log-sender`) containing one row per transmitted Message. Records only send-side data: sequence ID, payload size, wire size, and all send-side timestamps. Used to reconstruct the transmission chain and cross-reference against the Receiver's packets.csv when diagnosing gaps.
_Avoid_: sender metrics, sender output, sender trace

### Metrics

**End-to-End Delay**:
The time elapsed from when the Sender stamps `ts_sent` to when the Receiver stamps `ts_received`, corrected for the NTP clock offset difference between the two machines.
_Avoid_: latency, RTT, round-trip time

**Jitter**:
The absolute difference in End-to-End Delay between two consecutively received Messages (in sequence order).
_Avoid_: delay variation, latency variation

**Out-of-Order**:
A received Message whose sequence ID is strictly less than the last received sequence ID.
_Avoid_: reordered, misordered

**Sequence Gap**:
The difference `seq_id − (last_received_seq_id + 1)` for a received Message. A value greater than zero indicates the number of Messages presumed lost between this Message and the previous one.
_Avoid_: loss, missing packets

## Analysis

**Analysis Session**:
A single invocation of the analyzer tool (`analyzer/`) that selects one or more Runs, validates comparability, and produces a set of output figures (histograms, scatter plots, time series) in a timestamped output subfolder.
_Avoid_: comparison run, plot session

**Comparable Runs**:
Two or more Runs whose `message_rate_hz`, `payload_size_bytes`, and `duration_s` are identical. Only Comparable Runs may be overlaid in the same Analysis Session. Selecting incompatible Runs is a hard error.
_Avoid_: compatible runs, matching runs

**Run Label**:
The folder name of a Run output directory (`{run_id}_{protocol}_{serializer}_{compression}`), used as the display identifier in the analyzer listing table and plot legends.
_Avoid_: run name, experiment label

**Latency Dimension**:
One of the six pipeline stages for which per-packet latency is recorded and individually plotted: `e2e`, `serialization`, `compression`, `transport`, `decompression`, `deserialization`.
_Avoid_: metric, latency type

**Incomplete Run**:
A Run folder that is missing one or more of `config.json`, `summary.json`, or `packets.csv`. Shown in the analyzer listing table but excluded from selection.
_Avoid_: broken run, partial run
