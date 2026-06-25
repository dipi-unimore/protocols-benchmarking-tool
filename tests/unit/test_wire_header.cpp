#include <gtest/gtest.h>
#include "pbt/core/WireHeader.hpp"
#include "pbt/core/FragmentHeader.hpp"
#include <cstring>

using namespace pbt;

TEST(WireHeader, Size) {
    EXPECT_EQ(sizeof(WireHeader), 72u);
}

TEST(FragmentHeader, Size) {
    EXPECT_EQ(sizeof(FragmentHeader), 24u);
}

TEST(WireHeader, RoundTrip) {
    WireHeader h;
    h.sequence_id   = 12345;
    h.payload_size  = 256;
    h.original_size = 512;
    h.payload_crc32 = 0xDEADBEEFu;
    h.ts_created_ns = 999888777;
    h.set_warmup();

    uint8_t buf[sizeof(WireHeader)];
    std::memcpy(buf, &h, sizeof(WireHeader));

    WireHeader h2;
    std::memcpy(&h2, buf, sizeof(WireHeader));

    EXPECT_EQ(h2.magic,         0x50425401u);
    EXPECT_EQ(h2.sequence_id,   12345u);
    EXPECT_EQ(h2.payload_size,  256u);
    EXPECT_EQ(h2.original_size, 512u);
    EXPECT_EQ(h2.payload_crc32, 0xDEADBEEFu);
    EXPECT_EQ(h2.ts_created_ns, 999888777);
    EXPECT_TRUE(h2.is_warmup());
    EXPECT_FALSE(h2.is_sentinel());
    EXPECT_FALSE(h2.is_fragment());
}

TEST(WireHeader, Flags) {
    WireHeader h;
    EXPECT_FALSE(h.is_sentinel());
    EXPECT_FALSE(h.is_fragment());
    EXPECT_FALSE(h.is_warmup());
    h.set_sentinel();
    EXPECT_TRUE(h.is_sentinel());
    h.set_fragment();
    EXPECT_TRUE(h.is_fragment());
    h.set_warmup();
    EXPECT_TRUE(h.is_warmup());
    EXPECT_EQ(h.flags, 0x07u);
}
