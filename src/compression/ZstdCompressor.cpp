#include "pbt/compression/ZstdCompressor.hpp"
#include <format>
#include <fstream>
#include <stdexcept>

namespace pbt {

ZstdCompressor::ZstdCompressor(int level, const std::string& dict_path)
    : cctx_(ZSTD_createCCtx()), dctx_(ZSTD_createDCtx()) {
    if (!cctx_ || !dctx_) throw std::runtime_error("ZSTD context creation failed");
    ZSTD_CCtx_setParameter(cctx_.get(), ZSTD_c_compressionLevel, level);

    if (!dict_path.empty()) {
        std::ifstream f(dict_path, std::ios::binary);
        if (!f) throw std::runtime_error("Cannot open zstd dict: " + dict_path);
        dict_data_ = Bytes(std::istreambuf_iterator<char>(f), {});
        cdict_ = ZSTD_createCDict(dict_data_.data(), dict_data_.size(), level);
        ddict_ = ZSTD_createDDict(dict_data_.data(), dict_data_.size());
        if (!cdict_ || !ddict_)
            throw std::runtime_error("ZSTD dictionary creation failed");
    }
}

ZstdCompressor::~ZstdCompressor() {
    if (cdict_) ZSTD_freeCDict(cdict_);
    if (ddict_) ZSTD_freeDDict(ddict_);
}

std::expected<Bytes, Error>
ZstdCompressor::compress(std::span<const uint8_t> in) {
    Bytes out(ZSTD_compressBound(in.size()));
    std::size_t result;
    if (cdict_) {
        result = ZSTD_compress_usingCDict(cctx_.get(),
                                          out.data(), out.size(),
                                          in.data(),  in.size(), cdict_);
    } else {
        result = ZSTD_compressCCtx(cctx_.get(),
                                    out.data(), out.size(),
                                    in.data(),  in.size(), 3);
    }
    if (ZSTD_isError(result))
        return std::unexpected(Error{
            std::format("ZSTD compress: {}", ZSTD_getErrorName(result))});
    out.resize(result);
    return out;
}

std::expected<Bytes, Error>
ZstdCompressor::decompress(std::span<const uint8_t> in, std::size_t original_size) {
    Bytes out(original_size);
    std::size_t result;
    if (ddict_) {
        result = ZSTD_decompress_usingDDict(dctx_.get(),
                                             out.data(), out.size(),
                                             in.data(),  in.size(), ddict_);
    } else {
        result = ZSTD_decompressDCtx(dctx_.get(),
                                      out.data(), out.size(),
                                      in.data(),  in.size());
    }
    if (ZSTD_isError(result))
        return std::unexpected(Error{
            std::format("ZSTD decompress: {}", ZSTD_getErrorName(result))});
    out.resize(result);
    return out;
}

}  // namespace pbt
