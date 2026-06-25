#include <gtest/gtest.h>
#include "pbt/core/BenchmarkConfig.hpp"

using namespace pbt;

TEST(BenchmarkConfig, RoundTrip) {
    BenchmarkConfig cfg;
    cfg.protocol    = Protocol::Udp;
    cfg.serializer  = SerFmt::Protobuf;
    cfg.compression = Compression::Zstd;
    cfg.host        = "192.168.1.1";
    cfg.port        = 5000;
    cfg.duration_s  = 60;
    cfg.metadata["env"] = "lab";

    auto j   = cfg.to_json();
    auto res = BenchmarkConfig::from_json(j);
    ASSERT_TRUE(res.has_value()) << res.error().message;
    EXPECT_EQ(res->protocol,    cfg.protocol);
    EXPECT_EQ(res->serializer,  cfg.serializer);
    EXPECT_EQ(res->compression, cfg.compression);
    EXPECT_EQ(res->host,        cfg.host);
    EXPECT_EQ(res->port,        cfg.port);
    EXPECT_EQ(res->duration_s,  cfg.duration_s);
    EXPECT_EQ(res->metadata,    cfg.metadata);
}

TEST(BenchmarkConfig, CborBinaryRejected) {
    BenchmarkConfig cfg;
    cfg.serializer    = SerFmt::Cbor;
    cfg.payload_format = PayloadFmt::Binary;
    auto v = cfg.validate();
    EXPECT_FALSE(v.has_value());
}

TEST(BenchmarkConfig, CborRandomRejected) {
    BenchmarkConfig cfg;
    cfg.serializer    = SerFmt::Cbor;
    cfg.payload_format = PayloadFmt::Random;
    EXPECT_FALSE(cfg.validate().has_value());
}

TEST(BenchmarkConfig, CborJsonAccepted) {
    BenchmarkConfig cfg;
    cfg.serializer    = SerFmt::Cbor;
    cfg.payload_format = PayloadFmt::Json;
    EXPECT_TRUE(cfg.validate().has_value());
}
