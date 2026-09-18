#include "thread_pool.h"

using namespace plugins;

void ThreadPool::work() {
    std::size_t seen = m_wakeups.load(std::memory_order_acquire);
    for (;;) {
        std::function<void(void)> task;
        while (m_tasks->try_pop(task)) {
            // 任务异常由 packaged_task 捕获并写入 future；此处兜底，
            // 避免任何意外异常逃出线程函数导致 std::terminate。
            try {
                task();
            } catch (...) {
            }
            task = nullptr;
        }

        if (m_stop.load(std::memory_order_acquire))
            return;

        const std::size_t current = m_wakeups.load(std::memory_order_acquire);
        if (current == seen) {
            m_wakeups.wait(seen, std::memory_order_acquire);
            seen = m_wakeups.load(std::memory_order_acquire);
        } else {
            seen = current;
        }
    }
}

void ThreadPool::initAndStart(size_t nums, size_t queue_capacity) {
    if (nums == 0)
        nums = 1;
    if (m_start.exchange(true, std::memory_order_acq_rel))
        return;
    // 在启动工作线程前按需扩容，避免生产者因队列满而自旋。
    if (queue_capacity > m_tasks->capacity()) {
        m_tasks = std::make_unique<TaskQueue>(
            TaskQueue::roundCapacity(queue_capacity));
    }
    m_stop.store(false, std::memory_order_release);
    m_size = nums;
    m_workers.reserve(nums);
    for (size_t i = 0; i < nums; ++i) {
        m_workers.emplace_back(&ThreadPool::work, this);
    }
}

void ThreadPool::stop() {
    if (m_stop.exchange(true, std::memory_order_acq_rel))
        return;
    m_wakeups.fetch_add(1, std::memory_order_release);
    m_wakeups.notify_all();

    for (auto &t : m_workers) {
        if (t.joinable())
            t.join();
    }
}
