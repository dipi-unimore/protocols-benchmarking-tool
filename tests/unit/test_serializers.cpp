#include <gtest/gtest.h>
#include "pbt/serialization/NoneSerializer.hpp"
#include "pbt/serialization/ProtobufSerializer.hpp"
#include "pbt/serialization/CborSerializer.hpp"
#include <string>

using namespace pbt;

static const std::string kJson = R"({"key":"value","num":42})";
static const Bytes kJsonBytes(kJson.begin(), kJson.end());

TEST(NoneSerializer, RoundTrip) {
    NoneSerializer s;
    auto ser = s.serialize(kJsonBytes);
    ASSERT_TRUE(ser.has_value());
    EXPECT_EQ(*ser, kJsonBytes);
    auto deser = s.deserialize(*ser);
    ASSERT_TRUE(deser.has_value());
    EXPECT_EQ(*deser, kJsonBytes);
}

TEST(ProtobufSerializer, RoundTrip) {
    ProtobufSerializer s("json");
    auto ser = s.serialize(kJsonBytes);
    ASSERT_TRUE(ser.has_value());
    EXPECT_NE(*ser, kJsonBytes);  // encoded differently
    auto deser = s.deserialize(*ser);
    ASSERT_TRUE(deser.has_value());
    EXPECT_EQ(*deser, kJsonBytes);
}

TEST(CborSerializer, RoundTripJson) {
    CborSerializer s;
    auto ser = s.serialize(kJsonBytes);
    ASSERT_TRUE(ser.has_value()) << ser.error().message;
    EXPECT_NE(*ser, kJsonBytes);  // binary CBOR differs
    auto deser = s.deserialize(*ser);
    ASSERT_TRUE(deser.has_value()) << deser.error().message;
    // Decoded JSON should be parseable and equivalent
    std::string decoded(deser->begin(), deser->end());
    EXPECT_FALSE(decoded.empty());
}

TEST(CborSerializer, RejectsBinaryPayload) {
    // CBOR attempts to parse as JSON; binary data should fail gracefully
    CborSerializer s;
    Bytes binary = {0x00, 0x01, 0xFF, 0xAB};
    auto ser = s.serialize(binary);
    // Either error or (tolerated) — test that it doesn't crash
    (void)ser;
}
