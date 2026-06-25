#include <gtest/gtest.h>
#include "pbt/compression/NoopCompressor.hpp"
#include "pbt/compression/ZstdCompressor.hpp"

using namespace pbt;

static const Bytes kData = {0x48,0x65,0x6C,0x6C,0x6F,0x20,0x57,0x6F,0x72,0x6C,0x64};

TEST(NoopCompressor, RoundTrip) {
    NoopCompressor c;
    auto cmp = c.compress(kData);
    ASSERT_TRUE(cmp.has_value());
    EXPECT_EQ(*cmp, kData);
    auto dcmp = c.decompress(*cmp, kData.size());
    ASSERT_TRUE(dcmp.has_value());
    EXPECT_EQ(*dcmp, kData);
}

TEST(ZstdCompressor, RoundTrip) {
    ZstdCompressor c;
    auto cmp = c.compress(kData);
    ASSERT_TRUE(cmp.has_value()) << cmp.error().message;
    auto dcmp = c.decompress(*cmp, kData.size());
    ASSERT_TRUE(dcmp.has_value()) << dcmp.error().message;
    EXPECT_EQ(*dcmp, kData);
}

TEST(ZstdCompressor, LargerPayload) {
    ZstdCompressor c;
    Bytes large(4096, 0xAB);
    auto cmp = c.compress(large);
    ASSERT_TRUE(cmp.has_value());
    // Compressible data should compress
    EXPECT_LT(cmp->size(), large.size());
    auto dcmp = c.decompress(*cmp, large.size());
    ASSERT_TRUE(dcmp.has_value());
    EXPECT_EQ(*dcmp, large);
}
