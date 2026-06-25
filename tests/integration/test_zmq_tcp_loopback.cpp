#include <gtest/gtest.h>
#include "pbt/core/BenchmarkConfig.hpp"
#include "pbt/core/BoundedBlockingQueue.hpp"
#include "pbt/core/InboundPacket.hpp"
#include "pbt/payload/RandomSource.hpp"
#include "pbt/serialization/NoneSerializer.hpp"
#include "pbt/compression/NoopCompressor.hpp"
#include "pbt/transport/ZmqTcpSender.hpp"
#include "pbt/transport/ZmqTcpReceiver.hpp"
#include <thread>
#include <chrono>

using namespace pbt;
using namespace std::chrono_literals;

TEST(ZmqTcpLoopback, PushPull50) {
    BenchmarkConfig cfg;
    cfg.protocol    = Protocol::ZmqTcp;
    cfg.host        = "127.0.0.1";
    cfg.port        = 19702;
    cfg.zmq_pattern = ZmqPattern::PushPull;

    BoundedBlockingQueue<InboundPacket> queue(1024);

    ZmqTcpReceiver receiver(cfg, queue);
    ASSERT_TRUE(receiver.bind().has_value());
    ASSERT_TRUE(receiver.start().has_value());
    std::this_thread::sleep_for(100ms);

    auto src = std::make_unique<RandomSource>(64);
    ZmqTcpSender sender(cfg, std::move(src),
                        std::make_unique<NoneSerializer>(),
                        std::make_unique<NoopCompressor>());
    ASSERT_TRUE(sender.connect().has_value());
    std::this_thread::sleep_for(50ms);

    constexpr int kMsgs = 50;
    for (int i = 0; i < kMsgs; ++i) {
        auto r = sender.send(static_cast<uint64_t>(i));
        ASSERT_TRUE(r.has_value()) << r.error().message;
    }
    sender.disconnect();

    std::this_thread::sleep_for(200ms);
    receiver.stop();
    queue.stop();

    int received = 0;
    while (auto pkt = queue.pop()) ++received;
    EXPECT_GT(received, 0);
}
