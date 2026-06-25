#pragma once
#include "pbt/core/Types.hpp"
#include <expected>
#include <memory>
#include <span>

namespace pbt {

struct BenchmarkConfig;

class PayloadSource {
public:
    virtual ~PayloadSource() = default;

    [[nodiscard]] virtual std::span<const uint8_t> next() noexcept = 0;
    [[nodiscard]] virtual std::size_t record_count() const noexcept = 0;

    static std::expected<std::unique_ptr<PayloadSource>, Error>
        create(const BenchmarkConfig& cfg);
};

}  // namespace pbt
