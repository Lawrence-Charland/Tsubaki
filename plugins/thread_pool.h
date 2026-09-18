#ifndef _PLUGIN_THREAD_POOL_H_
#define _PLUGIN_THREAD_POOL_H_

#include "lock_free_queue.h"
#include "plugin.h"
#include <atomic>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace plugins {

template <typename F>
concept VoidCallable = requires(F f) {
    { f() };
};

class ThreadPool : public Plugin<ThreadPool> {

    using TaskQueue = LockFreeQueue<std::function<void(void)>>;

    // 默认无锁队列容量，必须是 2 的幂。
    static constexpr std::size_t kDefaultQueueCapacity = 1 << 14;

    // 队列在 initAndStart 中按需要的容量创建。
    std::unique_ptr<TaskQueue> m_tasks;
    std::vector<std::thread> m_workers;
    std::atomic_bool m_stop{false};
    std::atomic_bool m_start{false};
    std::size_t m_size = 0;
    // 每次入队或停止都会自增，工作线程在队列为空时据此休眠与唤醒。
    alignas(64) std::atomic<std::size_t> m_wakeups{0};

    void work();

  public:
    enum class exceptions {
        NONE,
        POOL_STOPPED,
    };

    ThreadPool()
        : m_tasks(std::make_unique<TaskQueue>(kDefaultQueueCapacity)) {}

    // 启动工作线程。queue_capacity 会向上取整到 2 的幂；仅当它大于
    // 现有容量时才重建队列，从而可按文件数预留足够空间。
    void initAndStart(size_t = std::thread::hardware_concurrency(),
                      size_t queue_capacity = kDefaultQueueCapacity);

    void stop();

    ~ThreadPool() { stop(); }

    template <typename F>
    std::future<std::invoke_result_t<F>> submit(F &&f) {
        using T = std::invoke_result_t<F>;
        // 停止检查必须位于 packaged_task 内部，异常才会被存入 future，
        // 否则会逃出工作线程并触发 std::terminate。
        auto task = std::make_shared<std::packaged_task<T()>>(
            [this, fn = std::forward<F>(f)]() mutable -> T {
                if (m_stop)
                    throw exceptions::POOL_STOPPED;
                return std::invoke(fn);
            });

        std::future<T> fut = task->get_future();

        if (m_stop.load(std::memory_order_acquire))
            throw std::runtime_error("ThreadPool is stopped");

        std::function<void(void)> job = [task]() { (*task)(); };
        // 队列满时让出 CPU，等待工作线程腾出空位。
        while (!m_tasks->try_push(std::move(job)))
            std::this_thread::yield();

        m_wakeups.fetch_add(1, std::memory_order_release);
        m_wakeups.notify_one();
        return fut;
    }

    // 提交一个无返回值的任务：不创建 packaged_task/future，开销更小。
    // 线程池已停止时返回 false，任务不会被执行。
    template <typename F> bool post(F &&f) {
        static_assert(std::is_void_v<std::invoke_result_t<F>>,
                      "ThreadPool::post requires a callable returning void");
        if (m_stop.load(std::memory_order_acquire))
            return false;

        std::function<void(void)> job = [this, fn = std::forward<F>(f)]() mutable {
            // 停止后跳过未执行的任务，使中断能尽快结束。
            if (m_stop)
                return;
            std::invoke(fn);
        };
        // 队列满时让出 CPU，等待工作线程腾出空位。
        while (!m_tasks->try_push(std::move(job)))
            std::this_thread::yield();

        m_wakeups.fetch_add(1, std::memory_order_release);
        m_wakeups.notify_one();
        return true;
    }
};

}; // namespace plugins
#endif
