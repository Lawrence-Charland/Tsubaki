#include "scan.h"
#include "common/common.h"
#include "plugins/arg_parser.h"
#include "plugins/file_list.h"
#include "plugins/logger.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

namespace fs = std::filesystem;

namespace {

void scanFile(std::string_view path, plugins::FileList &fl) {
    std::error_code ec;
    // 与目录扫描保持一致，解析符号链接，便于 --exclude 前缀匹配。
    fs::path abs = fs::weakly_canonical(path, ec);
    if (ec) {
        ec.clear();
        abs = fs::absolute(path, ec).lexically_normal();
    }
    if (ec) {
        plugins::Logger::GetInstance()(plugins::LogLevel::WARN, "SUM: {} is not accessible.",
                              path);
        return;
    }

    std::error_code size_ec;
    const auto size = fs::file_size(abs, size_ec);
    if (size_ec) {
        fl.add(abs.string());
    } else {
        fl.add(abs.string(), size);
    }
}

void scanDictionary(std::string_view path_str, plugins::FileList &fl) {
    const auto &parser = plugins::ArgParser::GetInstance();
    const auto &logger = plugins::Logger::GetInstance();

    std::error_code ec;
    fs::path path = fs::canonical(path_str, ec);
    if (ec) {
        logger(plugins::LogLevel::WARN, "Dictionary {} not found. Skipped.", path_str);
        return;
    }

    bool allow_symlinks = parser.getValue("--allow-symlinks") != nullptr;
    auto options = fs::directory_options::skip_permission_denied;
    if (allow_symlinks)
        options |= fs::directory_options::follow_directory_symlink;

    try {
        for (const auto &entry :
             fs::recursive_directory_iterator(path, options)) {
            std::error_code status_ec;
            const bool regular = entry.is_regular_file(status_ec);
            if (!status_ec && regular) {
                std::error_code size_ec;
                const auto size = entry.file_size(size_ec);
                if (size_ec) {
                    fl.add(entry.path().string());
                } else {
                    fl.add(entry.path().string(), size);
                }
            } else if (allow_symlinks && entry.is_symlink(status_ec)) {
                fl.add(entry.path().string());
            }
        }
    } catch (const fs::filesystem_error &msg) {
        logger(plugins::LogLevel::ERROR, "filesystem error while traversing {} : {}",
               path_str, msg.what());
    }
}

void scanStdinList(plugins::FileList &fl) {
    const auto &logger = plugins::Logger::GetInstance();
    logger(plugins::LogLevel::INFO,
           "--> SUM: Loading file list (plain list format) from stdin ...");

    std::string line;
    while (std::getline(std::cin, line)) {
        const auto begin = line.find_first_not_of(" \t\r\f\v");
        if (begin == std::string::npos || line[begin] == '#')
            continue;
        const auto end = line.find_last_not_of(" \t\r\f\v");
        fl.add(line.substr(begin, end - begin + 1));
    }
}

} // namespace

void sum::scan(std::string_view input, plugins::FileList &list) {
    const auto &logger = plugins::Logger::GetInstance();
    if (input == "stdin") {
        logger(plugins::LogLevel::INFO, "SUM: Will load file list (tsubaki "
                               "file list format) from stdin...\n");
        common::loadFilesFromStream(list, std::cin, "stdin", "SUM");
    } else if (input == "stdin-plain-list") {
        logger(plugins::LogLevel::INFO,
               "SUM: Will load file list (plain list format) from stdin...");
        scanStdinList(list);
    } else {
        std::error_code ec;
        const auto status = fs::status(input, ec);
        if (!ec && fs::is_regular_file(status)) {
            logger(plugins::LogLevel::INFO,
                   "SUM: {} is a regular file.\nCaculating {} hash of this "
                   "file.",
                   input, plugins::ArgParser::GetInstance().getCommands()[1]);
            scanFile(input, list);
        } else if (!ec && fs::is_directory(status)) {
            logger(plugins::LogLevel::INFO,
                   "SUM: {} is a dictionary.\nCaculating {} hash of all files "
                   "in the dictionary.",
                   input, plugins::ArgParser::GetInstance().getCommands()[1]);
            scanDictionary(input, list);
        } else {
            logger(plugins::LogLevel::WARN,
                   "SUM: {} is not a regular file or a directory; it will be "
                   "skiped.",
                   input);
        }
    }
}
