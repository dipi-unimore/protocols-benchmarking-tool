#include <gtest/gtest.h>
#include "pbt/core/BenchmarkConfig.hpp"
#include "pbt/core/BoundedBlockingQueue.hpp"
#include "pbt/core/InboundPacket.hpp"
#include "pbt/payload/RandomSource.hpp"
#include "pbt/serialization/NoneSerializer.hpp"
#include "pbt/compression/NoopCompressor.hpp"
#include "pbt/transport/TcpSender.hpp"
#include "pbt/transport/TcpReceiver.hpp"
#include <chrono>
#include <thread>

using namespace pbt;
using namespace std::chrono_literals;

TEST(TcpLoopback, SendReceive100) {
    BenchmarkConfig cfg;
    cfg.protocol = Protocol::Tcp;
    cfg.host     = "127.0.0.1";
    cfg.port     = 19700;

    BoundedBlockingQueue<InboundPacket> queue(1024);

    TcpReceiver receiver(cfg, queue);
    ASSERT_TRUE(receiver.bind().has_value());
    ASSERT_TRUE(receiver.start().has_value());
    std::this_thread::sleep_for(50ms);

    auto src = std::make_unique<RandomSource>(64);
    TcpSender sender(cfg, std::move(src),
                     std::make_unique<NoneSerializer>(),
                     std::make_unique<NoopCompressor>());
    ASSERT_TRUE(sender.connect().has_value());

    constexpr int kMsgs = 100;
    for (int i = 0; i < kMsgs; ++i) {
        auto r = sender.send(static_cast<uint64_t>(i));
        ASSERT_TRUE(r.has_value()) << r.error().message;
    }
    sender.disconnect();

    // Drain
    std::this_thread::sleep_for(200ms);
    receiver.stop();
    queue.stop();

    int received = 0;
    while (auto pkt = queue.pop()) ++received;
    EXPECT_EQ(received, kMsgs);
}
