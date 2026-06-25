# pb-tool — Protocol Benchmarking Tool

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Cross-machine benchmarking framework for IoT/mobility communication protocols. Measures end-to-end latency, jitter, packet loss, and throughput across the full serialization + compression + transport pipeline, with nanosecond-resolution timestamps and NTP-corrected cross-machine delay.

## Quick Start

```bash
# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.logicalcpu)   # macOS
# cmake --build build -j$(nproc)                    # Linux

# Terminal 1 — Receiver (wait for [READY])
./build/app/pb-tool --mode receiver --protocol tcp --port 5555 --run-id test01

# Terminal 2 — Sender
./build/app/pb-tool --mode sender --protocol tcp \
  --host 127.0.0.1 --port 5555 \
  --payload-format random --payload-size 512 \
  --rate 500 --duration 10 --run-id test01

# Results in ./results/test01_tcp_none_none/
```

## Documentation

| Section | File |
|---------|------|
| General info & pipeline | [docs/overview.md](docs/overview.md) |
| Architecture & UML diagrams | [docs/architecture.md](docs/architecture.md) |
| Build | [docs/build.md](docs/build.md) |
| Running & CLI reference | [docs/running.md](docs/running.md) |
| Result analysis (Python) | [docs/analyzer.md](docs/analyzer.md) |
| Extending the tool | [docs/extending.md](docs/extending.md) |
| Architectural decisions | [docs/adr/](docs/adr/) |
| Glossary | [CONTEXT.md](CONTEXT.md) |
