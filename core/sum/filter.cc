#include "filter.h"
#include "plugins/arg_parser.h"
#include "plugins/file_list.h"
#include "plugins/logger.h"
#include "plugins/trie.h"
#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace {
std::optional<uintmax_t> parseSize(std::string_view key) {
    const auto &logger = plugins::Logger::GetInstance();
    const auto &parser = plugins::ArgParser::GetInstance();
    auto size_item = parser.getValue(key);
    if (!size_item || size_item->empty()) {
        return std::nullopt;
    }

    if (size_item->size() >= 2) {
        logger(plugins::LogLevel::WARN,
               "More than 1 (received {}) sizes are provided. Skipper.",
               size_item->size());
    }
    std::string_view sv = size_item->front();
    double value = 0;
    auto last = sv.data() + sv.size();
    auto [ptr, ec] = std::from_chars(sv.data(), last, value);
    if (ec != std::errc{}) {
        logger(plugins::LogLevel::WARN, "Cannot parse the size: {}", sv);
        return std::nullopt;
    }
    if (value < 0) {
        logger(plugins::LogLevel::WARN, "The value of size {} if negative.",
               sv);
        return std::nullopt;
    }
    if (ptr == last) {
        return static_cast<long long int>(std::llround(value));
    }
    char unit = *ptr;
    long long scale = 1;
    switch (unit) {
    case 'b':
    case 'B':
        break;
    case 'K':
    case 'k':
        scale = 1ll << 10;
        break;
    case 'm':
    case 'M':
        scale = 1ll << 20;
        break;
    case 'g':
    case 'G':
        scale = 1ll << 30;
        break;
    case 't':
    case 'T':
        scale = 1ll << 40;
        break;
    case 'p':
    case 'P':
        scale = 1ll << 50;
        break;
    default:
        logger(plugins::LogLevel::WARN,
               "The unit {} in size {} is not supported. You can type "
               "'k','m','g','t','p'.",
               unit, sv);
        return std::nullopt;
    }

    return static_cast<long long>(std::llround(value * scale));
}
} // namespace

void sum::filter(plugins::FileList &fl) {
    const auto &parser = plugins::ArgParser::GetInstance();
    const auto &logger = plugins::Logger::GetInstance();
    plugins::Trie trie;
    const auto *excs = parser.getValue("--exclude");
    bool do_exc = excs && !excs->empty();
    if (do_exc) {
        std::error_code exc_ec;
        for (const auto &ex : *excs) {
            // 与扫描时一致地解析符号链接，否则在 /var -> /private/var
            // 这类系统上绝对路径的 --exclude 永远匹配不上。
            auto path = std::filesystem::weakly_canonical(ex, exc_ec);
            if (exc_ec) {
                exc_ec.clear();
                path = std::filesystem::absolute(ex, exc_ec).lexically_normal();
            }
            trie.insert(path.string());
        }
    }

    uintmax_t max_size = UINTMAX_MAX;
    if (auto ret = parseSize("--max-size"); ret) {
        max_size = *ret;
    }
    uintmax_t min_size = 0;
    if (auto ret = parseSize("--min-size"); ret) {
        min_size = *ret;
    }

    fl.initSizes();
    const size_t size = fl.size();

    using val_t = plugins::FileList::filesum_t::value_type;
    fl.eraseIf([&](const val_t &item) {
        const auto &[key, val] = item;
        if (do_exc && trie.match(key)) {
            return true;
        }
        if (val.size > max_size || val.size < min_size) {
            return true;
        }
        return false;
    });
    logger(plugins::LogLevel::INFO, "-->SUM: Ignored {} files.",
           size - fl.size());
}
