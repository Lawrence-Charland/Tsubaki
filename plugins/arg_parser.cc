#include "arg_parser.h"
#include <sstream>
#include <string>

namespace plugins {

void ArgParser::parse(int argc, char **argv) {
    m_argc = argc;
    m_argv = argv;

    for (int i = 1; i < argc; ++i) {

        std::string_view sv{argv[i]};

        if (sv.starts_with('-')) {
            const auto pos = sv.find('=');
            if (pos == std::string_view::npos) {
                options.try_emplace(sv);
            } else {
                const std::string_view key = sv.substr(0, pos);
                const std::string_view val = sv.substr(pos + 1);
                options[key].push_back(val);
            }
        } else {
            commands.push_back(sv);
        }
    }
}
const std::vector<std::string_view> *
ArgParser::getValue(std::string_view key) const {
    auto it = options.find(key);
    if (it != options.cend())
        return &(it->second);
    else
        return nullptr;
}

const std::vector<std::string_view> &ArgParser::getCommands() const {
    return commands;
}
const std::string ArgParser::getCommand() const {

    std::ostringstream res;

    for (int i = 0; i < m_argc; ++i) {
        if (i != 0)
            res << ' ';
        res << m_argv[i];
    }
    return res.str();
}

}; // namespace plugins
