#include <gtest/gtest.h>
#include "pbt/core/BoundedBlockingQueue.hpp"
#include <thread>

using namespace pbt;

TEST(BoundedBlockingQueue, BasicPushPop) {
    BoundedBlockingQueue<int> q(10);
    EXPECT_TRUE(q.try_push(42));
    auto v = q.pop();
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 42);
}

TEST(BoundedBlockingQueue, DropWhenFull) {
    BoundedBlockingQueue<int> q(1);
    EXPECT_TRUE(q.try_push(1));
    EXPECT_FALSE(q.try_push(2));  // full
    EXPECT_EQ(q.size(), 1u);
}

TEST(BoundedBlockingQueue, BlocksUntilItem) {
    BoundedBlockingQueue<int> q(10);
    std::optional<int> result;

    std::thread consumer([&] { result = q.pop(); });
    std::this_thread::sleep_for(std::chrono::milliseconds{20});
    q.try_push(99);
    consumer.join();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 99);
}

TEST(BoundedBlockingQueue, StopUnblocksConsumer) {
    BoundedBlockingQueue<int> q(10);
    std::optional<int> result{42};  // non-nullopt initial

    std::thread consumer([&] { result = q.pop(); });
    std::this_thread::sleep_for(std::chrono::milliseconds{20});
    q.stop();
    consumer.join();

    EXPECT_FALSE(result.has_value());  // nullopt after stop+empty
}
