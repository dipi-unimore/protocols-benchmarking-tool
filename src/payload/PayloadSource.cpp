#include "pbt/payload/PayloadSource.hpp"
#include "pbt/payload/FileSource.hpp"
#include "pbt/payload/RandomSource.hpp"
#include "pbt/core/BenchmarkConfig.hpp"
#include <format>

namespace pbt {

std::expected<std::unique_ptr<PayloadSource>, Error>
PayloadSource::create(const BenchmarkConfig& cfg) {
    if (cfg.payload_file.empty() || cfg.payload_format == PayloadFmt::Random)
        return std::make_unique<RandomSource>(cfg.payload_size_bytes);
    try {
        switch (cfg.payload_format) {
            case PayloadFmt::Text:
                return std::make_unique<PlainTextSource>(cfg.payload_file);
            case PayloadFmt::Json:
                return std::make_unique<JsonSource>(cfg.payload_file);
            case PayloadFmt::Yaml:
                return std::make_unique<YamlSource>(cfg.payload_file);
            case PayloadFmt::Kv:
                return std::make_unique<KeyValueSource>(cfg.payload_file);
            case PayloadFmt::Binary:
                return std::make_unique<BinarySource>(cfg.payload_file);
            default:
                return std::make_unique<RandomSource>(cfg.payload_size_bytes);
        }
    } catch (const std::exception& e) {
        return std::unexpected(Error{e.what()});
    }
}

}  // namespace pbt
