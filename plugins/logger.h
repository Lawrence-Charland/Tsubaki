#ifndef _PLUGINS_LOGGER_H_
#define _PLUGINS_LOGGER_H_

#include "plugin.h"
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace plugins {

#define PLUGIN_LOG_LEVELS(macro)                                               \
    macro(DEBUG) macro(INFO) macro(WARN) macro(ERROR)

enum class LogLevel {
#define LIST(name) name,
    NONE,
    PLUGIN_LOG_LEVELS(LIST)
#undef LIST
};

const size_t BUFFER_SIZE = 4 * 1024;

LogLevel parseLevel(std::string_view level);

inline auto operator<=>(LogLevel lhs, LogLevel rhs) {
    return static_cast<int>(lhs) <=> static_cast<int>(rhs);
}

// Logger 不是线程安全的
class Logger : public Plugin<Logger> {
  private:
    const bool m_color_enabled;
    void add(LogLevel level, const std::string &content) const;

  public:
    Logger();
    LogLevel m_min_level = LogLevel::INFO;

    inline void operator()(LogLevel level, const std::string &s) const {
        if (level < m_min_level || level == LogLevel::NONE)
            return;
        this->add(level, std::move(s));
    }
    template <typename... Args>
    void operator()(LogLevel level, std::format_string<Args...> s,
                    Args &&...args) const {
        if (level < m_min_level || level == LogLevel::NONE)
            return;
        this->add(level, std::format(s, std::forward<Args>(args)...));
    }
};

}; // namespace plugins

#endif // !_PLUGINS_LOGGER_H_
