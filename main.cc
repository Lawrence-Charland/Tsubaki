#include "arg_parser.h"
#include "cmp/cmp.h"
#include "common/common.h"
#include "common/help.h"
#include "dup/dup.h"
#include "logger.h"
#include "platform.h"
#include "sum/sum.h"
#include <iostream>
#include <string_view>

namespace {

// 在 argv 中查找 'help' / 'help-cn' 之后的主题键，支持 --key=value 写法。
std::string_view findHelpTopic(int argc, char *argv[]) {
    for (int i = 1; i + 1 < argc; ++i) {
        const std::string_view cur{argv[i]};
        if (cur != "help" && cur != "help-cn")
            continue;
        std::string_view next{argv[i + 1]};
        const auto pos = next.find('=');
        return pos == std::string_view::npos ? next : next.substr(0, pos);
    }
    return {};
}

} // namespace

int main(int argc, char *argv[]) {
    // 不混用 C stdio：关闭同步可显著提升逐行输出的吞吐。
    std::ios::sync_with_stdio(false);
    plugins::platform::enableUtf8Console();
    plugins::ArgParser &parser = plugins::ArgParser::GetInstance();
    const auto &logger = plugins::Logger::GetInstance();
    parser.parse(argc, argv);
    common::setLogLevel();

    if (parser.getValue("-h") || parser.getValue("--help")) {
        common::printHelp(false);
        return 0;
    }
    if (parser.getValue("--help-cn")) {
        common::printHelp(true);
        return 0;
    }

    const auto &commands = parser.getCommands();
    if (commands.empty()) {
        logger(plugins::LogLevel::ERROR,
               "At least one command is required.\nType 'tsubaki --help' for "
               "usage.");
        return 1;
    }

    const std::string_view command = commands.front();
    if (command == "help" || command == "help-cn") {
        const bool chinese = command == "help-cn";
        const std::string_view topic = findHelpTopic(argc, argv);
        if (topic.empty()) {
            common::printHelp(chinese);
            return 0;
        }
        if (!common::printHelpTopic(topic, chinese)) {
            logger(plugins::LogLevel::ERROR,
                   "Unknown help topic '{}'. Run '{}' to list the available "
                   "topics.",
                   topic, chinese ? "tsubaki help-cn" : "tsubaki help");
            return 1;
        }
        return 0;
    }
    if (command == "sum") {
        return sum::invoke();
    }
    if (command == "cmp") {
        return cmp::invoke();
    }
    if (command == "dup") {
        return duplicate::invoke();
    }

    logger(plugins::LogLevel::ERROR,
           "Unknown command '{}'. Type 'tsubaki --help' for usage.", command);
    return 1;
}
