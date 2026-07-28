# Build

## Prerequisites

| Requirement | Minimum version | Notes |
|-------------|----------------|-------|
| CMake | 3.28 | Required for `FetchContent` and C++23 support |
| C++ compiler | GCC 13 / Clang 16 / Apple Clang 15 | Full C++23 (`std::expected`, `std::jthread`, `std::format`) |
| Internet access | — | FetchContent downloads all dependencies at configure time |

All third-party libraries are fetched automatically — no manual installation required:

| Library | Purpose |
|---------|---------|
| `nlohmann_json` | JSON config parsing and output |
| `yaml-cpp` | YAML payload loading |
| `spdlog` | Structured logging |
| `GoogleTest` | Unit and integration tests |
| `protobuf` | Protobuf serialization + `protoc` |
| `zstd` | Zstd compression |
| `libzmq` + `cppzmq` | ZeroMQ transport |
| `paho-mqtt-cpp` (+ bundled `paho-mqtt-c`) | MQTT transport |
| `tinycbor` | CBOR serialization |

## Build Commands

### macOS (Apple Silicon or Intel)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.logicalcpu)
```

### Linux

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### ARM cross-compile (from x86_64 Linux)

```bash
cmake -B build-arm -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=cmake/arm-toolchain.cmake
cmake --build build-arm -j$(nproc)
```

The output binary is `build/app/pb-tool` (or `build-arm/app/pb-tool` for cross-compiled).

## Running Tests

```bash
ctest --test-dir build --output-on-failure
```

MQTT integration tests require a running broker. Export credentials before running:

```bash
export MQTT_HOST=localhost
export MQTT_PORT=1883
ctest --test-dir build --output-on-failure
```

Without these variables the MQTT tests are automatically skipped.

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `PBT_OUTPUT_DIR` | `./results` | Base directory for run output |
| `PBT_NTP_SERVER` | (probe `127.0.0.1`, then `pool.ntp.org`) | SNTP server hostname, same as `--ntp-server` |
| `PBT_NTP_OFFSET_NS` | (unset) | Manual NTP offset; skips the network query |
| `MQTT_HOST` | `localhost` | Used by integration tests |
| `MQTT_PORT` | `1883` | Used by integration tests |

Copy `.env.example` to `.env` and export variables before running:

```bash
cp .env.example .env
# edit .env
export $(grep -v '^#' .env | xargs)
```
