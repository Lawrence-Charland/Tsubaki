#include "cmp/cmp.h"
#include "common/common.h"
#include "plugins/arg_parser.h"
#include "plugins/logger.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Entry {
    std::string hash;
    std::string path;
    char type; // 'A' 或 'B'
    bool marked = false;
};

} // namespace

int cmp::compare(const std::string &file_a, const std::string &file_b,
                 std::ostream &out) {
    const auto &logger = plugins::Logger::GetInstance();

    std::ifstream stream_a(file_a), stream_b(file_b);
    if (!stream_a) {
        logger(plugins::LogLevel::ERROR, "CMP: Cannot read the file: {}",
               file_a);
        return 1;
    }
    if (!stream_b) {
        logger(plugins::LogLevel::ERROR, "CMP: Cannot read the file: {}",
               file_b);
        return 1;
    }

    std::vector<common::ChecksumEntry> raw_a, raw_b;
    std::vector<std::string> errors;
    common::loadChecksumEntries(stream_a, file_a, raw_a, errors);
    common::loadChecksumEntries(stream_b, file_b, raw_b, errors);

    std::vector<Entry> entries;
    entries.reserve(raw_a.size() + raw_b.size());
    for (auto &entry : raw_a)
        entries.push_back(
            {std::move(entry.hash), std::move(entry.path), 'A', false});
    for (auto &entry : raw_b)
        entries.push_back(
            {std::move(entry.hash), std::move(entry.path), 'B', false});

    std::vector<std::string> modified, matched;

    // 阶段一：同一路径在 A、B 中都出现时，比较哈希判断是否被修改。
    std::sort(entries.begin(), entries.end(),
              [](const Entry &a, const Entry &b) {
                  if (a.path != b.path)
                      return a.path < b.path;
                  return a.type < b.type;
              });
    for (std::size_t i = 0; i < entries.size();) {
        std::size_t j = i;
        bool has_a = false, has_b = false;
        const std::string *hash_a = nullptr;
        const std::string *hash_b = nullptr;
        while (j < entries.size() && entries[j].path == entries[i].path) {
            if (entries[j].type == 'A') {
                has_a = true;
                if (!hash_a)
                    hash_a = &entries[j].hash;
            } else {
                has_b = true;
                if (!hash_b)
                    hash_b = &entries[j].hash;
            }
            ++j;
        }
        if (has_a && has_b) {
            for (std::size_t k = i; k < j; ++k)
                entries[k].marked = true;
            if (*hash_a == *hash_b)
                matched.push_back(entries[i].path);
            else
                modified.push_back(entries[i].path);
        }
        i = j;
    }

    out << "[!] Modified:\n";
    for (const auto &path : modified)
        out << "[!] " << path << '\n';

    out << "\n[D] Moved,copied,merged or renamed:\n";

    // 阶段二：剩余记录按哈希分组；同一哈希在 A、B 都有则为移动/复制。
    std::vector<Entry *> rest;
    rest.reserve(entries.size());
    for (auto &entry : entries)
        if (!entry.marked)
            rest.push_back(&entry);
    std::sort(rest.begin(), rest.end(), [](const Entry *a, const Entry *b) {
        return a->hash < b->hash;
    });

    std::vector<std::string> a_deleted, b_added;
    int group = 0;
    for (std::size_t i = 0; i < rest.size();) {
        std::size_t j = i + 1;
        while (j < rest.size() && rest[j]->hash == rest[i]->hash)
            ++j;
        std::vector<const std::string *> in_a, in_b;
        for (std::size_t k = i; k < j; ++k) {
            if (rest[k]->type == 'A')
                in_a.push_back(&rest[k]->path);
            else
                in_b.push_back(&rest[k]->path);
        }
        if (in_a.empty()) {
            for (const auto *path : in_b)
                b_added.push_back(*path);
        } else if (in_b.empty()) {
            for (const auto *path : in_a)
                a_deleted.push_back(*path);
        } else {
            ++group;
            out << "[D][" << group << "]\n";
            out << "[D][" << group << "][A]\n";
            for (const auto *path : in_a)
                out << "[D][" << group << "][A]" << *path << '\n';
            out << "[D][" << group << "][B]\n";
            for (const auto *path : in_b)
                out << "[D][" << group << "][B]" << *path << '\n';
        }
        i = j;
    }

    out << "\n[U] Deleted or added:\n[U][A]\n";
    for (const auto &path : a_deleted)
        out << "[U][A]" << path << '\n';
    out << "[U][B]\n";
    for (const auto &path : b_added)
        out << "[U][B]" << path << '\n';

    out << "\n[=] Matched:\n";
    for (const auto &path : matched)
        out << "[=] " << path << '\n';

    if (!errors.empty()) {
        out << "\n[E] Error messages:\n";
        for (const auto &error : errors)
            out << "[E] " << error << '\n';
        logger(plugins::LogLevel::ERROR,
               "CMP: Several errors were reported. See the end of stdout.");
        return 1;
    }
    return 0;
}

int cmp::invoke() {
    const auto &parser = plugins::ArgParser::GetInstance();
    const auto &commands = parser.getCommands();
    if (commands.size() < 3) {
        plugins::Logger::GetInstance()(
            plugins::LogLevel::ERROR,
            "CMP: Two files are required.\nUsage: tsubaki cmp <fileA> <fileB>");
        return 1;
    }
    return compare(std::string(commands[1]), std::string(commands[2]),
                   std::cout);
}
