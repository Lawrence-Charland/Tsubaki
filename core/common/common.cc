#include "common.h"
#include "plugins/arg_parser.h"
#include "plugins/logger.h"
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <string>
#include <string_view>

std::string common::formatFileSize(uintmax_t bytes) {
    static constexpr const char *units[] = {"B",   "KiB", "MiB", "GiB",
                                            "TiB", "PiB", "EiB"};
    constexpr int unitCount = sizeof(units) / sizeof(units[0]);

    if (bytes == 0)
        return "0 B";

    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && unitIndex < unitCount - 1) {
        size /= 1024.0;
        ++unitIndex;
    }
    return std::format("{:.2f} {}", size, units[unitIndex]);
}

void common::setLogLevel() {

    const auto &parser = plugins::ArgParser::GetInstance();
    auto &logger = plugins::Logger::GetInstance();
    if (parser.getValue("--quiet")) {
        logger(plugins::LogLevel::WARN,
               "Key --quiet is deprecated. Use --log-level=ERROR instead.");
        logger.m_min_level = plugins::LogLevel::ERROR;
    } else if (parser.getValue("-v")) {
        if (logger.m_min_level == plugins::LogLevel::ERROR) {
            logger(plugins::LogLevel::ERROR,
                   "Key --quiet and -v are both set.");
            exit(1);
        } else {
            logger(plugins::LogLevel::WARN,
                   "Key -v is deprecated. Use --log-level=INFO instead");
        }
    } else if (const auto level = parser.getValue("--log-level"); level) {
        if (level->size() >= 2) {
            logger(plugins::LogLevel::ERROR,
                   "the key --log-level has {} values (more than 1).",
                   level->size());
        } else if (level->size() == 1) {
            plugins::LogLevel lev = plugins::parseLevel(level->front());
            if (lev == plugins::LogLevel::NONE) {
                logger(plugins::LogLevel::ERROR,
                       "the value of --log-level '{}' is not supported",
                       level->front());
            }
            logger.m_min_level = lev;
        } else {
            logger(plugins::LogLevel::ERROR,
                   "the key --log-level has no value");
        }
    }
}

bool checkIsValid(std::string_view view) {
    if (view.starts_with('<') && view.ends_with('>')) {
        return true;
    }
    for (auto c : view) {
        if ((c > '9' || c < '0') && (c > 'f' || c < 'a')) [[unlikely]]
            return false;
    }
    return true;
}

void common::loadFilesFromStream(plugins::FileList &fl, std::istream &is,
                                 std::string_view name, std::string_view mode) {

    const auto &logger = plugins::Logger::GetInstance();
    logger(plugins::LogLevel::INFO, "-->{}: Loading file list from {} ...",
           mode, name);

    std::string tmp;
    for (size_t line = 0; std::getline(is, tmp); ++line) {
        if (tmp.empty())
            continue;

        size_t pos = tmp.find_first_not_of(" \t\r\f\v");
        if (pos == std::string::npos || tmp[pos] == '#' || tmp[pos] == '[')
            continue;
        size_t space_pos = tmp.find_first_of(' ', pos);
        if (space_pos == std::string::npos) {
            logger(plugins::LogLevel::ERROR,
                   "{} in line {}: A directory must follow the checksum.", name,
                   line);
            return;
        }
        std::string_view view{tmp};
        std::string_view hash = view.substr(pos, space_pos - pos);
        if (!checkIsValid(hash)) {
            logger(plugins::LogLevel::ERROR, "{} in line {}: invalid hex value",
                   name, line);
            return;
        }
        fl.set(view.substr(space_pos + 1), hash);
    }
}

void common::loadChecksumEntries(std::istream &is, std::string_view source,
                                 std::vector<ChecksumEntry> &out,
                                 std::vector<std::string> &errors) {
    std::string line;
    std::size_t line_no = 0;
    while (std::getline(is, line)) {
        ++line_no;
        const auto begin = line.find_first_not_of(" \t\r\f\v");
        if (begin == std::string::npos || line[begin] == '#' ||
            line[begin] == '[')
            continue;
        const auto end = line.find_last_not_of(" \t\r\f\v");
        const std::string_view view{line.data() + begin, end - begin + 1};

        const auto space = view.find_first_of(" \t");
        if (space == std::string_view::npos) {
            errors.push_back(std::format(
                "{}: In line {}: a directory must follow the checksum.", source,
                line_no));
            continue;
        }
        const std::string_view hash = view.substr(0, space);
        if (!checkIsValid(hash)) {
            errors.push_back(std::format(
                "{}: In line {}: '{}' is not a checksum.", source, line_no,
                hash));
            continue;
        }
        const auto path_begin = view.find_first_not_of(" \t", space);
        if (path_begin == std::string_view::npos) {
            errors.push_back(std::format(
                "{}: In line {}: a directory must follow the checksum.", source,
                line_no));
            continue;
        }
        out.push_back({std::string(hash), std::string(view.substr(path_begin)),
                       0});
    }
}
