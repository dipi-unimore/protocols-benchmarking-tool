# pb-tool — Protocol Benchmarking Tool

Cross-machine benchmarking framework for IoT/mobility communication protocols. Measures end-to-end latency, jitter, packet loss, and throughput across the full serialization + compression + transport pipeline, with nanosecond-resolution timestamps and NTP-corrected cross-machine delay. Developed for the **MASA** (Modena Automotive Smart Area) research initiative.

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
| Info generali & pipeline | [docs/overview.md](docs/overview.md) |
| Architettura & diagrammi UML | [docs/architecture.md](docs/architecture.md) |
| Compilazione | [docs/build.md](docs/build.md) |
| Esecuzione & CLI reference | [docs/running.md](docs/running.md) |
| Analisi risultati (Python) | [docs/analyzer.md](docs/analyzer.md) |
| Estendere il tool | [docs/extending.md](docs/extending.md) |
| Decisioni architetturali | [docs/adr/](docs/adr/) |
| Glossario | [CONTEXT.md](CONTEXT.md) |
