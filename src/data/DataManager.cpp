#include "pbt/data/DataManager.hpp"
#include <cstdlib>
#include <fstream>
#include <format>
#include <stdexcept>

namespace pbt {

DataManager DataManager::from_env(const std::string& run_id,
                                   const BenchmarkConfig& cfg) {
    const char* env = std::getenv("PBT_OUTPUT_DIR");
    std::filesystem::path base = env ? env : "./results";
    DataManager dm(base / run_folder_name(run_id, cfg));
    return dm;
}

DataManager::DataManager(std::filesystem::path output_dir)
    : dir_(std::move(output_dir)) {}

std::expected<void, Error> DataManager::init(const std::string& run_id,
                                              const BenchmarkConfig& cfg) {
    std::error_code ec;
    std::filesystem::create_directories(dir_, ec);
    if (ec)
        return std::unexpected(Error{
            std::format("Cannot create output dir {}: {}", dir_.string(), ec.message())});

    auto j = cfg.to_json();
    j["run_id"] = run_id;
    std::ofstream f(dir_ / "config.json");
    if (!f)
        return std::unexpected(Error{"Cannot write config.json"});
    f << j.dump(2) << '\n';
    return {};
}

std::expected<void, Error> DataManager::save_result(const RunResult& r) {
    std::ofstream f(dir_ / "summary.json");
    if (!f)
        return std::unexpected(Error{"Cannot write summary.json"});
    f << r.to_json().dump(2) << '\n';
    return {};
}

}  // namespace pbt
