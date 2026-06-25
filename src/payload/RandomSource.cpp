#include "pbt/payload/RandomSource.hpp"
#include <random>

namespace pbt {

RandomSource::RandomSource(std::size_t size_bytes) : buf_(size_bytes) {
    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& b : buf_) b = static_cast<uint8_t>(dist(rng));
}

}  // namespace pbt
