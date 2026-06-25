#include <gtest/gtest.h>
#include "pbt/transport/ReassemblyBuffer.hpp"
#include "pbt/core/FragmentHeader.hpp"
#include <thread>

using namespace pbt;

TEST(ReassemblyBuffer, SingleFragment) {
    ReassemblyBuffer buf;
    Bytes received;
    Bytes payload = {1, 2, 3, 4, 5};
    FragmentHeader fhdr;
    fhdr.message_id     = 1;
    fhdr.frag_index     = 0;
    fhdr.frag_count     = 1;
    fhdr.frag_data_size = static_cast<uint32_t>(payload.size());
    buf.insert(fhdr, payload, [&](Bytes msg) { received = std::move(msg); });
    EXPECT_EQ(received, payload);
}

TEST(ReassemblyBuffer, MultiFragment) {
    ReassemblyBuffer buf;
    Bytes received;
    Bytes part0 = {1, 2, 3};
    Bytes part1 = {4, 5, 6};
    Bytes expected = {1, 2, 3, 4, 5, 6};

    FragmentHeader f0;
    f0.message_id = 42; f0.frag_index = 0; f0.frag_count = 2;
    f0.frag_data_size = 3;
    buf.insert(f0, part0, [&](Bytes m) { received = std::move(m); });
    EXPECT_TRUE(received.empty());

    FragmentHeader f1;
    f1.message_id = 42; f1.frag_index = 1; f1.frag_count = 2;
    f1.frag_data_size = 3;
    buf.insert(f1, part1, [&](Bytes m) { received = std::move(m); });
    EXPECT_EQ(received, expected);
}

TEST(ReassemblyBuffer, ExpiredFragment) {
    ReassemblyBuffer buf(std::chrono::milliseconds{1});
    Bytes received;
    Bytes part0 = {1, 2};
    FragmentHeader f0;
    f0.message_id = 99; f0.frag_index = 0; f0.frag_count = 2;
    f0.frag_data_size = 2;
    buf.insert(f0, part0, [&](Bytes m) { received = std::move(m); });

    std::this_thread::sleep_for(std::chrono::milliseconds{10});

    // Insert a second different message to trigger sweep
    Bytes p2 = {9};
    FragmentHeader f2;
    f2.message_id = 100; f2.frag_index = 0; f2.frag_count = 1;
    f2.frag_data_size = 1;
    buf.insert(f2, p2, [&](Bytes m) { received = std::move(m); });
    // message 100 completes (1 frag), message 99 should have expired
    EXPECT_GT(buf.timeout_count(), 0u);
}
