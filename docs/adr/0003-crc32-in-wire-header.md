# CRC32 integrity field in WireHeader, applied to all protocols

Every Message carries a `payload_crc32` field (uint32_t, CRC32) in the WireHeader covering the compressed payload bytes transmitted after the header. The Receiver validates it before attempting decompression, across all protocols — not only UDP.

We chose a header-embedded CRC over a trailer approach because it allows fail-fast validation with a single header read: the Receiver knows `payload_size` and `payload_crc32` from the header before reading any payload byte. A trailer would require reading the full payload first.

We applied it to all protocols (not only UDP) because application-level corruption can occur independently of transport-layer checksums — e.g., bugs in the fragmentation/reassembly logic, memory errors, or software faults. A uniform validation path simplifies the Receiver code and makes corruption detection protocol-agnostic.

## Consequences

- WireHeader is 72 bytes (not 64 as previously estimated). `static_assert(sizeof(WireHeader) == 72)` enforces this.
- CRC32 computation adds a small overhead on the Sender side (after compression, before `do_send`). This overhead is measurable and recorded in `ts_compressed_ns → ts_sent_ns` interval.
- A `crc_error_count` field is added to `RunResult` to track integrity failures without aborting the Run.
