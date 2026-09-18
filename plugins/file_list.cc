#include "file_list.h"
#include "logger.h"
#include "thread_pool.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <future>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using namespace plugins;

bool FileList::add(std::string file) {
    auto [it, inserted] = m_filesums.try_emplace(std::move(file));
    if (!inserted)
        return false;
    it->second.hash = "<NONE>";
    return true;
}

bool FileList::add(std::string file, uintmax_t size) {
    auto [it, inserted] = m_filesums.try_emplace(std::move(file));
    if (!inserted)
        return false;
    it->second.hash = "<NONE>";
    it->second.size = size;
    return true;
}

void FileList::set(std::string_view file, std::string_view hash) {
    m_filesums[std::string(file)].hash = hash;
}

void FileList::initSizes() {
    const auto &logger = Logger::GetInstance();

    std::vector<const std::string *> pending;
    for (const auto &[key, val] : m_filesums) {
        if (val.size == unknown_size)
            pending.push_back(&key);
    }
    if (pending.empty())
        return;

    const std::size_t count = pending.size();
    std::vector<uintmax_t> sizes(count);
    std::vector<std::error_code> errors(count);

    // 通过线程池并行获取文件大小，避免逐个阻塞。
    auto &pool = ThreadPool::GetInstance();
    pool.initAndStart();

    std::vector<std::future<void>> futures;
    futures.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        futures.push_back(pool.submit([&, i] {
            sizes[i] = std::filesystem::file_size(*pending[i], errors[i]);
        }));
    }
    for (auto &fut : futures) {
        try {
            fut.get();
        } catch (...) {
            // 具体错误已写入 errors，这里吞掉线程池相关异常。
        }
    }

    for (std::size_t i = 0; i < count; ++i) {
        if (errors[i]) {
            logger(LogLevel::WARN, "Get size of file {} error. Skipped.",
                   *pending[i]);
            m_filesums.erase(*pending[i]);
        } else {
            m_filesums[*pending[i]].size = sizes[i];
        }
    }
}

uintmax_t FileList::getTotalSize(bool skip_computed) {
    uintmax_t ret = 0;
    for (const auto &[key, val] : m_filesums) {
        if (val.size == unknown_size)
            continue;
        if (!skip_computed || val.hash.empty() || val.hash.front() == '<')
            ret += val.size;
    }
    return ret;
}
