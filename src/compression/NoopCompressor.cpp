#include "pbt/compression/NoopCompressor.hpp"
#include "pbt/compression/ZstdCompressor.hpp"

namespace pbt {

std::expected<Bytes, Error>
NoopCompressor::compress(std::span<const uint8_t> in) {
    return Bytes(in.begin(), in.end());
}

std::expected<Bytes, Error>
NoopCompressor::decompress(std::span<const uint8_t> in, std::size_t) {
    return Bytes(in.begin(), in.end());
}

// --- factory ---

std::unique_ptr<Compressor> Compressor::create(Compression c,
                                                const std::string& dict_path) {
    switch (c) {
        case Compression::None: return std::make_unique<NoopCompressor>();
        case Compression::Zstd: return std::make_unique<ZstdCompressor>(3, dict_path);
    }
    return std::make_unique<NoopCompressor>();
}

}  // namespace pbt
