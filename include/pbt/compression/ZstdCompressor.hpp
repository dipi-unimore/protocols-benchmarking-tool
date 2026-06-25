#pragma once
#include "Compressor.hpp"
#include <memory>
#include <zstd.h>

namespace pbt {

class ZstdCompressor final : public Compressor {
public:
    explicit ZstdCompressor(int level = 3, const std::string& dict_path = "");
    ~ZstdCompressor() override;

    std::expected<Bytes, Error> compress(std::span<const uint8_t> in) override;
    std::expected<Bytes, Error> decompress(std::span<const uint8_t> in,
                                            std::size_t original_size) override;
    Compression type() const noexcept override { return Compression::Zstd; }

private:
    struct CCtxDel { void operator()(ZSTD_CCtx* p) const { ZSTD_freeCCtx(p); } };
    struct DCtxDel { void operator()(ZSTD_DCtx* p) const { ZSTD_freeDCtx(p); } };
    std::unique_ptr<ZSTD_CCtx, CCtxDel> cctx_;
    std::unique_ptr<ZSTD_DCtx, DCtxDel> dctx_;
    Bytes       dict_data_;
    ZSTD_CDict* cdict_{nullptr};
    ZSTD_DDict* ddict_{nullptr};
};

}  // namespace pbt
