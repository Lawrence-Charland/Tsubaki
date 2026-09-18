#include "logger.h"
#include "platform.h"
#include <cstdio>
#include <format>
#include <iostream>
#include <ostream>

using namespace plugins;

namespace {
using plugins::LogLevel;
const char *levelColor(LogLevel level) {

    switch (level) {
    case LogLevel::DEBUG:
        return "\033[36m";
    case LogLevel::INFO:
        return "\033[32m"; // 绿
    case LogLevel::WARN:
        return "\033[33m"; // 黄
    case LogLevel::ERROR:
        return "\033[31m"; // 红
    default:
        return "\033[0m";
    }
}
const char *levelContent(LogLevel level) {
    switch (level) {

#define LEVEL_CASE(name)                                                       \
    case LogLevel::name:                                                       \
        return #name;

        PLUGIN_LOG_LEVELS(LEVEL_CASE)
#undef LEVEL_CASE
    default:
        return "NONE";
    }
}
}; // namespace

LogLevel plugins::parseLevel(std::string_view level) {
    if (0) {
    }
#define COMPARE_LEVEL(name)                                                    \
    else if (level == #name) {                                                 \
        return LogLevel::name;                                                 \
    }
    PLUGIN_LOG_LEVELS(COMPARE_LEVEL)
#undef COMPARE_LEVEL
    return LogLevel::NONE;
}

Logger::Logger() : m_color_enabled{platform::isTerminal(stderr)} {}

void Logger::add(LogLevel level, const std::string &content) const {
    if (level < m_min_level)
        return;

    std::ostream &s = (level >= LogLevel::WARN ? std::cerr : std::clog);
    if (m_color_enabled) {
        s << std::format("[{}{}\033[0m] {}\n", levelColor(level),
                         levelContent(level), content);
    } else {
        s << std::format("[{}] {}\n", levelContent(level), content);
    }
}
