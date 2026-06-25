#include <gtest/gtest.h>
#include "pbt/core/BenchmarkConfig.hpp"
#include "pbt/core/BoundedBlockingQueue.hpp"
#include "pbt/core/InboundPacket.hpp"
#include "pbt/payload/RandomSource.hpp"
#include "pbt/serialization/NoneSerializer.hpp"
#include "pbt/compression/NoopCompressor.hpp"
#include "pbt/transport/MqttTcpSender.hpp"
#include "pbt/transport/MqttTcpReceiver.hpp"
#include <cstdlib>
#include <thread>
#include <chrono>

using namespace pbt;
using namespace std::chrono_literals;

TEST(MqttTcpLoopback, SendReceive10) {
    const char* mqtt_host = std::getenv("MQTT_HOST");
    const char* mqtt_port = std::getenv("MQTT_PORT");
    if (!mqtt_host || !mqtt_port) {
        GTEST_SKIP() << "MQTT_HOST/MQTT_PORT not set; skipping MQTT loopback test";
    }

    BenchmarkConfig cfg;
    cfg.protocol   = Protocol::MqttTcp;
    cfg.host       = mqtt_host;
    cfg.port       = static_cast<uint16_t>(std::stoi(mqtt_port));
    cfg.mqtt_topic = "pbt/test/loopback";
    cfg.mqtt_qos   = QoS::AtMostOnce;

    BoundedBlockingQueue<InboundPacket> queue(1024);
    MqttTcpReceiver receiver(cfg, queue);
    ASSERT_TRUE(receiver.bind().has_value());
    ASSERT_TRUE(receiver.start().has_value());
    std::this_thread::sleep_for(300ms);

    auto src = std::make_unique<RandomSource>(64);
    MqttTcpSender sender(cfg, std::move(src),
                         std::make_unique<NoneSerializer>(),
                         std::make_unique<NoopCompressor>());
    ASSERT_TRUE(sender.connect().has_value());
    std::this_thread::sleep_for(100ms);

    constexpr int kMsgs = 10;
    for (int i = 0; i < kMsgs; ++i) {
        auto r = sender.send(static_cast<uint64_t>(i));
        ASSERT_TRUE(r.has_value()) << r.error().message;
    }
    std::this_thread::sleep_for(500ms);
    sender.disconnect();
    receiver.stop();
    queue.stop();

    int received = 0;
    while (auto pkt = queue.pop()) ++received;
    EXPECT_GT(received, 0);
}
