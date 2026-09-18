#include "dup/dup.h"
#include "common/common.h"
#include "plugins/logger.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

// 将建议的 rm 命令按宽度折行输出，续行以反斜杠结尾，便于复制执行。
void printRmCommand(std::ostream &out,
                    const std::vector<std::string> &paths) {
    constexpr std::size_t kMaxWidth = 80;
    out << "\nRecommended clearing command:\n";
    std::string line = "rm";
    bool has_path = false;
    for (const auto &path : paths) {
        const std::string token = " '" + path + "'";
        if (has_path && line.size() + token.size() > kMaxWidth) {
            out << line << " \\\n";
            line = "   ";
        }
        line += token;
        has_path = true;
    }
    out << line << '\n';
}

// 同一路径只保留首次出现的记录。
void removeDuplicatePaths(std::vector<common::ChecksumEntry> &entries) {
    std::unordered_set<std::string> seen;
    seen.reserve(entries.size());
    std::vector<common::ChecksumEntry> kept;
    kept.reserve(entries.size());
    for (auto &entry : entries) {
        if (seen.insert(entry.path).second)
            kept.push_back(std::move(entry));
    }
    entries = std::move(kept);
}

} // namespace

int duplicate::run(std::istream &in, std::ostream &out) {
    const auto &logger = plugins::Logger::GetInstance();

    std::vector<common::ChecksumEntry> entries;
    std::vector<std::string> errors;
    common::loadChecksumEntries(in, "stdin", entries, errors);

    if (entries.empty()) {
        logger(plugins::LogLevel::WARN,
               "DUP: No checksum entries were loaded from stdin.");
        return errors.empty() ? 0 : 1;
    }

    removeDuplicatePaths(entries);
    std::sort(entries.begin(), entries.end(),
              [](const common::ChecksumEntry &a,
                 const common::ChecksumEntry &b) { return a.hash < b.hash; });

    int group = 0;
    std::vector<std::string> recommend;
    for (std::size_t i = 0; i < entries.size();) {
        std::size_t j = i + 1;
        while (j < entries.size() && entries[j].hash == entries[i].hash)
            ++j;
        if (j - i > 1) {
            ++group;
            out << '[' << group << "] Checksum: " << entries[i].hash << '\n';
            out << '[' << group << "] " << entries[i].path << '\n';
            for (std::size_t k = i + 1; k < j; ++k) {
                out << '[' << group << "] " << entries[k].path << '\n';
                recommend.push_back(entries[k].path);
            }
            out << '\n';
        }
        i = j;
    }

    if (group > 0) {
        printRmCommand(out, recommend);
    } else {
        logger(plugins::LogLevel::INFO, "DUP: No duplicates were found.");
    }

    if (!errors.empty()) {
        out << "\n[E] Error messages:\n";
        for (const auto &error : errors)
            out << "[E] " << error << '\n';
        logger(plugins::LogLevel::ERROR,
               "DUP: Several errors were reported. See the end of stdout.");
        return 1;
    }
    return 0;
}

int duplicate::invoke() { return run(std::cin, std::cout); }
