#pragma once
#include "pbt/core/BenchmarkConfig.hpp"
#include "pbt/core/RunResult.hpp"
#include <expected>
#include <filesystem>
#include <string>

namespace pbt {

class DataManager {
public:
    static DataManager from_env(const std::string& run_id, const BenchmarkConfig& cfg);

    explicit DataManager(std::filesystem::path output_dir);

    std::expected<void, Error> init(const std::string& run_id, const BenchmarkConfig& cfg);
    std::expected<void, Error> save_result(const RunResult& r);

    [[nodiscard]] const std::filesystem::path& output_dir() const noexcept { return dir_; }

private:
    std::filesystem::path dir_;
};

}  // namespace pbt
