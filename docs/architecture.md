# Architecture

## Transport Layer

```mermaid
classDiagram
    class Sender {
        <<abstract>>
        +connect() expected~void~
        +disconnect() expected~void~
        +send(seq_id, is_warmup) expected~WireHeader~
        +send_sentinel(last_seq_id) expected~void~
        +create(cfg, src, ser, cmp, ntp)$ unique_ptr~Sender~
        #do_send(wire)* expected~void~
        #assemble_wire(hdr, payload) Bytes
    }
    class TcpSender {
        -int fd_
        +connect()
        +disconnect()
        #do_send(wire)
    }
    class UdpSender {
        -int fd_
        +connect()
        +disconnect()
        #do_send(wire)
        -send_datagram(data)
    }
    class ZmqTcpSender {
        -zmq_context_t ctx_
        -zmq_socket_t socket_
        +connect()
        +disconnect()
        #do_send(wire)
    }
    class MqttTcpSender {
        -mqtt_async_client client_
        +connect()
        +disconnect()
        #do_send(wire)
    }

    Sender <|-- TcpSender
    Sender <|-- UdpSender
    Sender <|-- ZmqTcpSender
    Sender <|-- MqttTcpSender

    class Receiver {
        <<abstract>>
        +bind() expected~void~
        +start() expected~void~
        +stop() expected~void~
        +overflow_count() uint64_t
        +sentinel_received() bool
        +create(cfg, queue, ntp)$ unique_ptr~Receiver~
        #on_wire_bytes(wire)
        #parse_wire(wire, hdr, payload) bool
    }
    class TcpReceiver {
        -int server_fd_
        -jthread accept_thread_
        +bind()
        +start()
        +stop()
        -accept_loop(stop_token)
        -client_loop(fd, stop_token)
    }
    class UdpReceiver {
        -int fd_
        -jthread thread_
        -ReassemblyBuffer reassembly_
        +bind()
        +start()
        +stop()
        +fragment_timeout_count() size_t
        -recv_loop(stop_token)
    }
    class ZmqTcpReceiver {
        -zmq_context_t ctx_
        -zmq_socket_t socket_
        -jthread thread_
        +bind()
        +start()
        +stop()
        -recv_loop(stop_token)
    }
    class MqttTcpReceiver {
        -mqtt_async_client client_
        +bind()
        +start()
        +stop()
        -message_arrived(msg)
    }
    class ReassemblyBuffer {
        -milliseconds expiry_ms_
        -unordered_map slots_
        -size_t timeout_count_
        +insert(fhdr, data, cb) size_t
        +timeout_count() size_t
        -sweep_expired()
    }

    Receiver <|-- TcpReceiver
    Receiver <|-- UdpReceiver
    Receiver <|-- ZmqTcpReceiver
    Receiver <|-- MqttTcpReceiver
    UdpReceiver --> ReassemblyBuffer
```

## Serialization & Compression

```mermaid
classDiagram
    class Serializer {
        <<interface>>
        +serialize(payload) expected~Bytes~
        +deserialize(data) expected~Bytes~
        +format() SerFmt
        +create(fmt, source_format)$ unique_ptr~Serializer~
    }
    class NoneSerializer {
        +serialize(p)
        +deserialize(d)
        +format() None
    }
    class CborSerializer {
        +serialize(payload)
        +deserialize(data)
        +format() Cbor
    }
    class ProtobufSerializer {
        -string source_format_
        +serialize(payload)
        +deserialize(data)
        +format() Protobuf
    }

    Serializer <|-- NoneSerializer
    Serializer <|-- CborSerializer
    Serializer <|-- ProtobufSerializer

    class Compressor {
        <<interface>>
        +compress(in) expected~Bytes~
        +decompress(in, original_size) expected~Bytes~
        +type() Compression
        +create(c, dict_path)$ unique_ptr~Compressor~
    }
    class NoopCompressor {
        +compress(in)
        +decompress(in, size)
        +type() None
    }
    class ZstdCompressor {
        -int level_
        -unique_ptr~ZSTD_CCtx~ cctx_
        -unique_ptr~ZSTD_DCtx~ dctx_
        -ZSTD_CDict* cdict_
        -ZSTD_DDict* ddict_
        +compress(in)
        +decompress(in, size)
        +type() Zstd
    }

    Compressor <|-- NoopCompressor
    Compressor <|-- ZstdCompressor
```

## Payload Sources

```mermaid
classDiagram
    class PayloadSource {
        <<interface>>
        +next() span~uint8_t~
        +record_count() size_t
        +create(cfg)$ expected~unique_ptr~
    }
    class RandomSource {
        -Bytes buf_
        +next()
        +record_count() 1
    }
    class FileSource {
        -vector~Bytes~ records_
        -size_t idx_
        +next()
        +record_count()
    }
    class PlainTextSource {
        +PlainTextSource(path)
    }
    class JsonSource {
        +JsonSource(path)
    }
    class YamlSource {
        +YamlSource(path)
    }
    class KeyValueSource {
        +KeyValueSource(path)
    }
    class BinarySource {
        -Bytes buf_
        +next()
        +record_count() 1
    }

    PayloadSource <|-- RandomSource
    PayloadSource <|-- FileSource
    FileSource <|-- PlainTextSource
    FileSource <|-- JsonSource
    FileSource <|-- YamlSource
    FileSource <|-- KeyValueSource
    PayloadSource <|-- BinarySource
```

## Core Data Structures

```mermaid
classDiagram
    class WireHeader {
        +uint32_t magic
        +uint8_t flags
        +uint8_t serializer_id
        +uint8_t compressor_id
        +uint64_t sequence_id
        +uint32_t payload_size
        +uint32_t original_size
        +uint8_t qos
        +uint32_t payload_crc32
        +int64_t ts_created_ns
        +int64_t ts_serialized_ns
        +int64_t ts_compressed_ns
        +int64_t ts_sent_ns
        +int64_t ntp_offset_ns
        +int64_t ntp_uncertainty_ns
        +is_sentinel() bool
        +is_fragment() bool
        +is_warmup() bool
        +is_ntp_disabled() bool
    }
    class BenchmarkConfig {
        +Protocol protocol
        +string host
        +uint16_t port
        +SerFmt serializer
        +Compression compression
        +string zstd_dict_path
        +PayloadFmt payload_format
        +uint32_t payload_size_bytes
        +uint32_t message_rate_hz
        +uint32_t duration_s
        +uint32_t warmup_s
        +map~string,string~ metadata
        +validate() expected~void~
        +to_json() json
        +from_json(j)$ expected~BenchmarkConfig~
        +from_file(path)$ expected~BenchmarkConfig~
    }
    class LatencyStats {
        +double mean_us
        +double stddev_us
        +double min_us
        +double max_us
        +double p50_us
        +double p95_us
        +double p99_us
        +double p999_us
        +double jitter_us
    }
    class RunResult {
        +BenchmarkConfig config
        +optional~uint64_t~ msgs_sent
        +uint64_t msgs_received
        +uint64_t out_of_order_count
        +uint64_t duplicate_count
        +uint64_t crc_error_count
        +uint64_t overflow_count
        +double packet_loss_pct
        +double throughput_msgs_per_sec
        +double throughput_bytes_per_sec
        +LatencyStats e2e
        +LatencyStats serialization
        +LatencyStats compression
        +LatencyStats transport
        +LatencyStats decompression
        +LatencyStats deserialization
        +to_json() json
    }

    RunResult --> BenchmarkConfig
    RunResult --> LatencyStats : 6x
```

## Runtime Composition

### Sender side

```mermaid
classDiagram
    Sender o-- PayloadSource : owns
    Sender o-- Serializer : owns
    Sender o-- Compressor : owns
    Sender ..> BenchmarkConfig : const ref
    Sender ..> WireHeader : produces
    Sender ..> CsvSenderWriter : optional (--log-sender)
```

### Receiver side

```mermaid
classDiagram
    Receiver ..> BoundedBlockingQueue : ref
    Receiver ..> BenchmarkConfig : const ref
    PacketProcessor ..> BoundedBlockingQueue : ref
    PacketProcessor ..> Serializer : ref
    PacketProcessor ..> Compressor : ref
    PacketProcessor ..> CsvPacketWriter : ref
    PacketProcessor --> WelfordAccumulator : 7x
    PacketProcessor ..> RunResult : produces
    DataManager ..> RunResult : persists
```

## Sender Pipeline — Sequence Diagram

```mermaid
sequenceDiagram
    participant Main
    participant S as Sender
    participant PS as PayloadSource
    participant SER as Serializer
    participant CMP as Compressor
    participant NET as Socket

    Main->>S: send(seq_id, is_warmup)
    S->>PS: next()
    PS-->>S: span~uint8_t~
    S->>SER: serialize(payload)
    SER-->>S: Bytes
    S->>CMP: compress(bytes)
    CMP-->>S: Bytes
    S->>S: build WireHeader
    Note over S: ts_created, ts_serialized,\nts_compressed, ts_sent\nCRC32 over payload
    S->>NET: do_send(wire_bytes)
    S-->>Main: WireHeader
    opt --log-sender
        Main->>CsvSenderWriter: write_row(hdr, sizes)
    end
```

## Receiver Pipeline — Sequence Diagram

```mermaid
sequenceDiagram
    participant NET as Network
    participant RT as Receiver (net thread)
    participant Q as BoundedBlockingQueue
    participant PT as PacketProcessor (proc thread)
    participant CMP as Compressor
    participant SER as Serializer
    participant CSV as CsvPacketWriter
    participant DM as DataManager

    NET->>RT: raw bytes
    RT->>RT: stamp ts_received_ns
    RT->>RT: parse WireHeader
    RT->>RT: validate CRC32
    RT->>Q: try_push(InboundPacket)
    Note over Q: drops if full\noverflow_count++

    loop single processing thread
        Q->>PT: pop() — blocks until packet available
        PT->>CMP: decompress(payload)
        CMP-->>PT: Bytes
        PT->>SER: deserialize(bytes)
        SER-->>PT: Bytes
        PT->>PT: compute PacketMetrics\n(e2e, jitter, seq_gap, is_ooo)
        PT->>PT: update WelfordAccumulators\n+ reservoir sampling
        PT->>CSV: write_row(metrics)
    end

    RT-->>Q: sentinel packet → Q.stop()
    PT->>PT: finalize() → RunResult
    PT->>DM: save_result(result)
```

## NTP Clock Sync — Sequence Diagram

```mermaid
sequenceDiagram
    participant Main
    participant NTP as NtpSync
    participant Server as SNTP server

    Main->>NTP: query(server, samples=8, timeout=3s)
    loop 8 independent exchanges
        NTP->>Server: SNTP Request UDP/123 (T1=local send time)
        Server-->>NTP: SNTP Response (T2=server recv, T3=server send)
        Note over NTP: T4 = local recv time
        NTP->>NTP: offset = ((T2−T1)+(T3−T4))/2, rtt = (T4−T1)−(T3−T2)
    end
    NTP->>NTP: keep sample with min(rtt) — least path asymmetry
    NTP-->>Main: NtpInfo{offset_ns, uncertainty_ns = min_rtt/2}

    Note over Main: Sender: skips this entirely if --ntp-skip-loopback\nand --host is loopback (offset/uncertainty = 0,\nWireHeader.flags bit3 = ntp_disabled)
    Note over Main: Embedded in every WireHeader.ntp_offset_ns\n+ ntp_uncertainty_ns

    Note over Main: Receiver applies own offset at processing time
    Note over Main: E2E = ntp_disabled ? (ts_received − ts_sent)\n: (ts_received − ts_sent + receiver_offset − sender_offset)\n(µs, both offsets forced to 0 when ntp_disabled)
```

## CMake Module Dependency Graph

```mermaid
graph TD
    EXE[pb-tool] --> T[pbt_transport]
    EXE --> M[pbt_metrics]
    EXE --> D[pbt_data]
    EXE --> SY[pbt_sync]
    EXE --> C[pbt_core]

    T --> SER[pbt_serialization]
    T --> CMP[pbt_compression]
    T --> C
    T --> ZMQ[cppzmq-static]
    T --> MQTT[paho-mqttpp3-static]

    SER --> PR[pbt_proto]
    SER --> C

    CMP --> C
    CMP --> ZSTD[libzstd_static]

    M --> SER
    M --> CMP
    M --> D
    M --> C

    D --> C
    SY --> C

    PR --> PB[protobuf::libprotobuf]
```
