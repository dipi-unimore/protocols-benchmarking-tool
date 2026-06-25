#include <gtest/gtest.h>
#include "pbt/metrics/Statistics.hpp"
#include "pbt/metrics/WelfordAccumulator.hpp"
#include <cmath>
#include <numeric>

using namespace pbt;

TEST(Statistics, Percentile) {
    std::vector<double> v(100);
    std::iota(v.begin(), v.end(), 0.0);
    // already sorted
    double p50 = stats::percentile(v, 0.50);
    EXPECT_NEAR(p50, 49.5, 0.01);
    double p99 = stats::percentile(v, 0.99);
    EXPECT_NEAR(p99, 98.01, 0.1);
    double p0 = stats::percentile(v, 0.0);
    EXPECT_NEAR(p0, 0.0, 0.01);
}

TEST(Statistics, JitterConstant) {
    std::vector<double> v(100, 5.0);
    EXPECT_NEAR(stats::jitter(v), 0.0, 1e-9);
}

TEST(Statistics, JitterAlternating) {
    std::vector<double> v = {1.0, 3.0, 1.0, 3.0};
    EXPECT_NEAR(stats::jitter(v), 2.0, 1e-9);
}

TEST(Statistics, Compute) {
    std::vector<double> v(100);
    std::iota(v.begin(), v.end(), 1.0);  // [1..100]
    auto s = stats::compute(v);
    EXPECT_NEAR(s.mean_us, 50.5, 0.1);
    EXPECT_NEAR(s.min_us,   1.0, 0.01);
    EXPECT_NEAR(s.max_us, 100.0, 0.01);
    EXPECT_GT(s.stddev_us, 0.0);
}

TEST(WelfordAccumulator, Basic) {
    WelfordAccumulator acc;
    for (int i = 1; i <= 5; ++i) acc.update(static_cast<double>(i));
    EXPECT_EQ(acc.count(), 5u);
    EXPECT_NEAR(acc.mean(), 3.0, 1e-9);
    EXPECT_NEAR(acc.min(),  1.0, 1e-9);
    EXPECT_NEAR(acc.max(),  5.0, 1e-9);
    EXPECT_GT(acc.stddev(), 0.0);
}
