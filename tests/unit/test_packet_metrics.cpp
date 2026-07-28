#include <gtest/gtest.h>
#include "pbt/metrics/PacketMetrics.hpp"

using namespace pbt;

// Sender clock reads 50ms behind true time (needs +50ms to reach true time),
// receiver clock reads 10ms behind true time (needs +10ms), true network
// delay is 2ms. Sent/received raw timestamps carry the uncorrected skew.
TEST(PacketMetrics, E2eDelayCorrectsClockSkew) {
    constexpr int64_t sender_offset_ns   = 50'000'000;
    constexpr int64_t receiver_offset_ns = 10'000'000;
    constexpr int64_t true_send_ns       = 1'000'000'000;
    constexpr int64_t true_network_delay_ns = 2'000'000;

    // Raw local readings: local_reading = true_time - offset.
    const int64_t ts_sent_ns     = true_send_ns - sender_offset_ns;
    const int64_t ts_received_ns = (true_send_ns + true_network_delay_ns) - receiver_offset_ns;

    double e2e_us = compute_e2e_delay_us(ts_received_ns, ts_sent_ns,
                                          sender_offset_ns, receiver_offset_ns,
                                          /*ntp_disabled=*/false);

    EXPECT_NEAR(e2e_us, 2000.0, 1e-6);  // 2ms network delay, skew fully cancelled
}

TEST(PacketMetrics, E2eDelaySymmetricOffsetsCancel) {
    double e2e_us = compute_e2e_delay_us(/*ts_received_ns=*/1'005'000,
                                          /*ts_sent_ns=*/1'000'000,
                                          /*sender_offset_ns=*/30'000'000,
                                          /*receiver_offset_ns=*/30'000'000,
                                          /*ntp_disabled=*/false);
    EXPECT_NEAR(e2e_us, 5.0, 1e-6);  // equal offsets: no correction, just raw 5us delta
}

TEST(PacketMetrics, E2eDelayZeroOffsetsPassthrough) {
    double e2e_us = compute_e2e_delay_us(/*ts_received_ns=*/2'500'000,
                                          /*ts_sent_ns=*/1'000'000,
                                          /*sender_offset_ns=*/0,
                                          /*receiver_offset_ns=*/0,
                                          /*ntp_disabled=*/false);
    EXPECT_NEAR(e2e_us, 1500.0, 1e-6);
}

// Loopback auto-skip (main.cpp) and --no-ntp both flag the run as ntp_disabled on the wire;
// PacketMetrics must ignore any locally-computed offset in that case (e.g. a receiver that
// independently ran a real NTP query against a loopback sender) so cross-query sync noise can't
// leak into a run where there is no true clock offset to correct for in the first place.
TEST(PacketMetrics, NtpDisabledIgnoresNonZeroOffsets) {
    double e2e_us = compute_e2e_delay_us(/*ts_received_ns=*/2'500'000,
                                          /*ts_sent_ns=*/1'000'000,
                                          /*sender_offset_ns=*/50'000'000,
                                          /*receiver_offset_ns=*/-30'000'000,
                                          /*ntp_disabled=*/true);
    EXPECT_NEAR(e2e_us, 1500.0, 1e-6);  // offsets ignored entirely, pure raw delta
}
