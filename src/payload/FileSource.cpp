#include "pbt/payload/FileSource.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace pbt {

// --- PlainTextSource ---

PlainTextSource::PlainTextSource(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open: " + path.string());
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty())
            records_.emplace_back(line.begin(), line.end());
    }
    if (records_.empty()) throw std::runtime_error("No records in: " + path.string());
}

// --- JsonSource ---

JsonSource::JsonSource(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open: " + path.string());
    auto root = nlohmann::json::parse(f);
    if (root.is_array()) {
        for (auto& elem : root) {
            auto s = elem.dump();
            records_.emplace_back(s.begin(), s.end());
        }
    } else {
        auto s = root.dump();
        records_.emplace_back(s.begin(), s.end());
    }
    if (records_.empty()) throw std::runtime_error("No records in: " + path.string());
}

// --- YamlSource ---

YamlSource::YamlSource(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open: " + path.string());
    // Load all documents
    auto docs = YAML::LoadAll(f);
    for (auto& doc : docs) {
        // Emit as JSON string via nlohmann (YAML node → string → nlohmann parse)
        std::ostringstream oss;
        oss << doc;
        auto s = oss.str();
        records_.emplace_back(s.begin(), s.end());
    }
    if (records_.empty()) throw std::runtime_error("No records in: " + path.string());
}

// --- KeyValueSource ---

KeyValueSource::KeyValueSource(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open: " + path.string());
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty())
            records_.emplace_back(line.begin(), line.end());
    }
    if (records_.empty()) throw std::runtime_error("No records in: " + path.string());
}

// --- BinarySource ---

BinarySource::BinarySource(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open: " + path.string());
    buf_ = Bytes(std::istreambuf_iterator<char>(f), {});
    if (buf_.empty()) throw std::runtime_error("Empty file: " + path.string());
}

}  // namespace pbt
