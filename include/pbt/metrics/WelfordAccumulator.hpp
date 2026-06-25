#pragma once
#include <cmath>
#include <cstdint>
#include <limits>

namespace pbt {

class WelfordAccumulator {
public:
    void update(double x) noexcept {
        ++n_;
        double delta = x - mean_;
        mean_ += delta / static_cast<double>(n_);
        m2_ += delta * (x - mean_);
        if (x < min_) min_ = x;
        if (x > max_) max_ = x;
    }

    [[nodiscard]] uint64_t count()  const noexcept { return n_; }
    [[nodiscard]] double   mean()   const noexcept { return n_ ? mean_ : 0.0; }
    [[nodiscard]] double   stddev() const noexcept {
        return n_ > 1 ? std::sqrt(m2_ / static_cast<double>(n_ - 1)) : 0.0;
    }
    [[nodiscard]] double   min()    const noexcept {
        return n_ ? min_ : 0.0;
    }
    [[nodiscard]] double   max()    const noexcept {
        return n_ ? max_ : 0.0;
    }

    void reset() noexcept {
        n_    = 0;
        mean_ = 0.0;
        m2_   = 0.0;
        min_  = std::numeric_limits<double>::max();
        max_  = std::numeric_limits<double>::lowest();
    }

private:
    uint64_t n_{0};
    double   mean_{0.0};
    double   m2_{0.0};
    double   min_{std::numeric_limits<double>::max()};
    double   max_{std::numeric_limits<double>::lowest()};
};

}  // namespace pbt
