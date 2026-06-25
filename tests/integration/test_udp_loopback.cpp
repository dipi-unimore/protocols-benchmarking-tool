#include <gtest/gtest.h>
#include "pbt/core/BenchmarkConfig.hpp"
#include "pbt/core/BoundedBlockingQueue.hpp"
#include "pbt/core/InboundPacket.hpp"
#include "pbt/payload/RandomSource.hpp"
#include "pbt/serialization/NoneSerializer.hpp"
#include "pbt/compression/NoopCompressor.hpp"
#include "pbt/transport/UdpSender.hpp"
#include "pbt/transport/UdpReceiver.hpp"
#include <thread>
#include <chrono>

using namespace pbt;
using namespace std::chrono_literals;

TEST(UdpLoopback, SendReceive50) {
    BenchmarkConfig cfg;
    cfg.protocol = Protocol::Udp;
    cfg.host     = "127.0.0.1";
    cfg.port     = 19701;
    cfg.udp_mtu  = 1472;

    BoundedBlockingQueue<InboundPacket> queue(1024);

    UdpReceiver receiver(cfg, queue);
    ASSERT_TRUE(receiver.bind().has_value());
    ASSERT_TRUE(receiver.start().has_value());
    std::this_thread::sleep_for(50ms);

    auto src = std::make_unique<RandomSource>(64);
    UdpSender sender(cfg, std::move(src),
                     std::make_unique<NoneSerializer>(),
                     std::make_unique<NoopCompressor>());
    ASSERT_TRUE(sender.connect().has_value());

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
    // UDP may lose some on loopback, but not all
    EXPECT_GT(received, 0);
}
