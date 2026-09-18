#include "sum/sum.h"
#include "common/common.h"
#include "plugins/arg_parser.h"
#include "plugins/encoder.h"
#include "plugins/file_list.h"
#include "plugins/lock_free_queue.h"
#include "plugins/logger.h"
#include "plugins/progress_bar.h"
#include "plugins/thread_pool.h"
#include "scan.h"
#include "sum/filter.h"
#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <vector>
namespace {

// 仅由信号处理函数写入、主线程读取，必须为 sig_atomic_t
volatile std::sig_atomic_t g_interrupted = 0;

// worker 完成后把文件登记项放入完成队列，主线程取出并输出。
struct Completion {
    const std::string *key;
    plugins::FileList::FileInfo *val;
};

std::tuple<int, int, bool> calc(plugins::ThreadPool &pool,
                                plugins::FileList &fl,
                                const plugins::Logger &logger,
                                plugins::ProgressBar &bar,
                                uintmax_t total_bytes, const EVP_MD *md,
                                bool skip_if_computed) {
    int success = 0;
    int error = 0;
    bool interrupted = false;
    std::size_t done = 0;
    uintmax_t bytes_done = 0;

    auto tick = [&]() {
        ++done;
        if (!bar.enabled())
            return;
        bar.update(done,
                   std::format("{} / {}", common::formatFileSize(bytes_done),
                               common::formatFileSize(total_bytes)));
    };

    // 输出单个文件的结果并推进进度
    auto emit = [&](const std::string &key, plugins::FileList::FileInfo &val,
                    bool count_error) {
        bar.clear();
        if (!val.hash.empty() && val.hash.front() != '<') {
            std::cout << val.hash << ' ' << key << '\n';
            ++success;
        } else {
            std::cout << "<NONE> " << key << '\n';
            if (count_error)
                ++error;
        }
        if (val.size != plugins::FileList::unknown_size) {
            bytes_done += val.size;
        }
        tick();
    };

    // 完成队列：worker 算完即入队，主线程无需按顺序阻塞等待。
    // 容量取到能容纳全部文件，保证工作线程不会因队列满而自旋。
    const std::size_t completion_capacity = std::max<std::size_t>(
        1024, plugins::LockFreeQueue<Completion>::roundCapacity(fl.size()));
    plugins::LockFreeQueue<Completion> completed{completion_capacity};
    alignas(64) std::atomic<std::size_t> wakeups{0};
    std::size_t pending = 0;

    auto drain_ready = [&]() {
        Completion item;
        while (completed.try_pop(item)) {
            emit(*item.key, *item.val, true);
            --pending;
        }
    };

    // 收集待计算的文件；已知哈希的直接输出。
    std::vector<Completion> to_hash;
    to_hash.reserve(fl.size());
    for (auto &[key, val] : fl) {
        if (g_interrupted) {
            interrupted = true;
            break;
        }
        if (!md) {
            val.hash = "<NONE>";
            emit(key, val, true);
            continue;
        }
        if (!val.hash.empty() && val.hash.front() != '<') {
            if (skip_if_computed) {
                emit(key, val, true);
                continue;
            }
            val.hash.clear();
        }
        to_hash.push_back({&key, &val});
    }

    // LPT 调度：大文件先提交，避免收尾时只剩少数线程在算大文件。
    std::sort(to_hash.begin(), to_hash.end(),
              [](const Completion &a, const Completion &b) {
                  return a.val->size > b.val->size;
              });

    // 提交阶段：一次性提交，之后才收取结果。
    for (const Completion &item : to_hash) {
        if (g_interrupted) {
            interrupted = true;
            break;
        }
        const std::string *key_ptr = item.key;
        plugins::FileList::FileInfo *val_ptr = item.val;
        if (pool.post([&, key_ptr, val_ptr, md] {
                try {
                    val_ptr->hash = plugins::Encoder::encodeFile(
                        *key_ptr, val_ptr->size, md);
                } catch (...) {
                    logger(plugins::LogLevel::WARN,
                           "Error occured when calculating the sum of {}",
                           *key_ptr);
                    val_ptr->hash = "<NONE>";
                }
                while (!completed.try_push(Completion{key_ptr, val_ptr})) {
                    // 中断时丢弃结果，避免队列满导致工作线程无法退出。
                    if (g_interrupted)
                        return;
                    std::this_thread::yield();
                }
                wakeups.fetch_add(1, std::memory_order_release);
                wakeups.notify_one();
            })) {
            ++pending;
        }
    }

    // 收尾阶段：提交完成后才开始收取结果并输出，避免输出拖慢任务提交、
    // 导致工作线程无任务可做而空闲。
    std::size_t seen = wakeups.load(std::memory_order_acquire);
    while (pending > 0 && !g_interrupted) {
        drain_ready();
        if (pending == 0)
            break;
        if (wakeups.load(std::memory_order_acquire) == seen)
            wakeups.wait(seen, std::memory_order_acquire);
        seen = wakeups.load(std::memory_order_acquire);
    }
    if (g_interrupted)
        interrupted = true;

    bar.finish();
    pool.stop();
    return {success, error, interrupted};
}

// 信号处理函数只能执行异步信号安全操作，因此仅置标志位，
// 实际的线程池停止由主线程在 calc 返回后完成。
void sig(int) { g_interrupted = 1; }

} // namespace

int sum::invoke() {
    const auto &parser = plugins::ArgParser::GetInstance();
    const auto &encoder = plugins::Encoder::GetInstance();
    const auto &logger = plugins::Logger::GetInstance();

    const auto &commands = parser.getCommands();

    if (commands.size() < 2) {
        logger(plugins::LogLevel::ERROR,
               "Missing checksum algorithm.\nUsage: tsubaki sum <algorithm> "
               "<path...>");
        return 1;
    }
    if (commands.size() < 3) {
        logger(plugins::LogLevel::ERROR,
               "Missing input path.\nUsage: tsubaki sum <algorithm> <path...>");
        return 1;
    }

    std::signal(SIGINT, &sig);

    std::string_view sum_type = commands[1];
    const EVP_MD *md = encoder.getMdByName(sum_type);
    if (!md) {
        logger(plugins::LogLevel::WARN,
               "the sum type {} is not supported. All results will be recorded "
               "as <NONE>.",
               sum_type);
    }

    logger(plugins::LogLevel::INFO, "===SUM [SCAN]===");

    plugins::FileList list;
    for (auto it = commands.begin() + 2; it != commands.end(); ++it) {
        scan(*it, list);
    }

    logger(plugins::LogLevel::INFO, "");
    logger(plugins::LogLevel::INFO, "===SUM [FILTER]===");
    logger(plugins::LogLevel::INFO, "");

    // 线程数：默认按硬件并发数，可用 --threads=N 覆盖。
    unsigned threads = std::thread::hardware_concurrency();
    if (const auto *values = parser.getValue("--threads");
        values && !values->empty()) {
        if (values->size() >= 2) {
            logger(plugins::LogLevel::WARN,
                   "More than 1 (received {}) values for --threads are "
                   "provided. Only the first is used.",
                   values->size());
        }
        const std::string_view text = values->front();
        unsigned parsed = 0;
        const auto [ptr, ec] =
            std::from_chars(text.data(), text.data() + text.size(), parsed);
        if (ec != std::errc{} || ptr != text.data() + text.size() ||
            parsed == 0) {
            logger(plugins::LogLevel::WARN,
                   "Invalid value for --threads: '{}'. Using {} instead.", text,
                   threads);
        } else {
            threads = parsed;
        }
    }
    if (threads == 0)
        threads = 1;
    logger(plugins::LogLevel::INFO, "-->SUM: Using {} worker thread(s).",
           threads);

    // 线程池在 filter 之前启动：filter 中的文件大小统计也通过线程池完成，
    // 且队列有界，必须在提交任务前启动工作线程，否则会因队列满而死锁。
    auto &pool = plugins::ThreadPool::GetInstance();
    pool.initAndStart(threads, list.size());

    sum::filter(list);

    if (list.empty()) {
        logger(plugins::LogLevel::INFO,
               "==>SUM: No files matching the requirements were found. "
               "Calculation will not be started.");
        return md ? 0 : 1;
    }
    logger(plugins::LogLevel::INFO,
           "-->SUM: Eventually {} regular files were loaded.", list.size());
    logger(plugins::LogLevel::INFO, "");
    logger(plugins::LogLevel::INFO, "===SUM [CONFIG]===");
    logger(plugins::LogLevel::INFO, "");
    logger(plugins::LogLevel::INFO, "-->SUM: Force-rescan is {}.",
           (parser.getValue("--force-scan") ? "on" : "off"));

    bool skip_if_computed = !parser.getValue("--force-scan");
    uintmax_t size = list.getTotalSize(skip_if_computed);
    logger(plugins::LogLevel::INFO, "-->SUM: Total size is {} bytes ({})", size,
           common::formatFileSize(size));
    logger(plugins::LogLevel::INFO, "");
    logger(plugins::LogLevel::INFO, "===SUM [PROGRESS]===");
    logger(plugins::LogLevel::INFO, "");
    if (parser.getValue("--test")) {
        logger(plugins::LogLevel::INFO,
               "-->SUM: Calculation will not be started for argument: --test.");
        return 0;
    }
    logger(plugins::LogLevel::INFO, "-->SUM: Calculating checksums...");

    // 默认仅在 stderr 为终端、终端足够宽且日志级别不高于 INFO 时启用，
    // 可用 --progress / --no-progress 强制覆盖。
    // 终端过窄时进度条会换行并破坏 '\r' 原地刷新，因此自动禁用该功能。
    bool progress_enabled;
    if (parser.getValue("--no-progress")) {
        progress_enabled = false;
    } else if (parser.getValue("--progress")) {
        progress_enabled = true;
    } else {
        progress_enabled = logger.m_min_level <= plugins::LogLevel::INFO &&
                           plugins::ProgressBar::terminalIsWideEnough();
    }
    plugins::ProgressBar bar(list.size(), std::cerr, progress_enabled);

    auto start_time = std::chrono::system_clock::now();
    auto start_at = std::chrono::steady_clock::now();
    auto [succeed, error, interrupted] = calc(
        pool, list, logger, bar, list.getTotalSize(false), md, skip_if_computed);
    auto end_at = std::chrono::steady_clock::now();
    auto end_time = std::chrono::system_clock::now();

    const std::string interrupted_line =
        interrupted ? "# Interrupted: yes\n" : "";
    std::cout << std::format("# \n# ----------General Report----------\n"
                             "# Total: {}\n"
                             "# Succeed: {}\n"
                             "# Failed: {}\n"
                             "# Unprocessed: {}\n"
                             "{}"
                             "# Time started: {:%Y-%m-%d %H:%M:%S}\n"
                             "# Time finished: {:%Y-%m-%d %H:%M:%S}\n"
                             "# Duration: {}\n"
                             "# Command: {}",

                             list.size(), succeed, error,
                             list.size() - succeed - error, interrupted_line,
                             start_time, end_time, end_at - start_at,
                             parser.getCommand())
              << std::endl;

    if (interrupted) {
        return 130;
    }
    return error > 0 ? 1 : 0;
}
