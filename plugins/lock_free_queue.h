#ifndef _PLUGINS_LOCK_FREE_QUEUE_H_
#define _PLUGINS_LOCK_FREE_QUEUE_H_

#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

namespace plugins {

// 有界无锁 MPMC 队列（Vyukov 算法）。
// 容量必须是 2 的幂；try_push / try_pop 在队满 / 队空时返回 false，
// 全程不使用互斥锁，可被多个生产者与消费者并发调用。
template <typename T> class LockFreeQueue {
  public:
    explicit LockFreeQueue(std::size_t capacity)
        : m_capacity(capacity), m_mask(capacity - 1),
          m_cells(std::make_unique<Cell[]>(capacity)) {
        assert(capacity > 0 && (capacity & (capacity - 1)) == 0 &&
               "LockFreeQueue capacity must be a power of two");
        for (std::size_t i = 0; i < capacity; ++i)
            m_cells[i].seq.store(i, std::memory_order_relaxed);
    }

    LockFreeQueue(const LockFreeQueue &) = delete;
    LockFreeQueue &operator=(const LockFreeQueue &) = delete;

    // 返回不小于 n 的最小 2 的幂，便于构造合法容量。
    static std::size_t roundCapacity(std::size_t n) {
        std::size_t capacity = 1;
        while (capacity < n)
            capacity <<= 1;
        return capacity;
    }

    // 入队。仅在成功时移动 value；队列已满时返回 false 且 value 保持不变。
    bool try_push(T &&value) {
        std::size_t pos = m_enqueue.load(std::memory_order_relaxed);
        for (;;) {
            Cell &cell = m_cells[pos & m_mask];
            const std::size_t seq = cell.seq.load(std::memory_order_acquire);
            const auto dif = static_cast<std::ptrdiff_t>(seq) -
                             static_cast<std::ptrdiff_t>(pos);
            if (dif == 0) {
                if (m_enqueue.compare_exchange_weak(
                        pos, pos + 1, std::memory_order_relaxed))
                    break;
            } else if (dif < 0) {
                return false;
            } else {
                pos = m_enqueue.load(std::memory_order_relaxed);
            }
        }
        Cell &cell = m_cells[pos & m_mask];
        cell.data = std::move(value);
        cell.seq.store(pos + 1, std::memory_order_release);
        return true;
    }

    // 出队。队列为空时返回 false 且 value 保持不变。
    bool try_pop(T &value) {
        std::size_t pos = m_dequeue.load(std::memory_order_relaxed);
        for (;;) {
            Cell &cell = m_cells[pos & m_mask];
            const std::size_t seq = cell.seq.load(std::memory_order_acquire);
            const auto dif = static_cast<std::ptrdiff_t>(seq) -
                             static_cast<std::ptrdiff_t>(pos + 1);
            if (dif == 0) {
                if (m_dequeue.compare_exchange_weak(
                        pos, pos + 1, std::memory_order_relaxed))
                    break;
            } else if (dif < 0) {
                return false;
            } else {
                pos = m_dequeue.load(std::memory_order_relaxed);
            }
        }
        Cell &cell = m_cells[pos & m_mask];
        value = std::move(cell.data);
        cell.seq.store(pos + m_mask + 1, std::memory_order_release);
        return true;
    }

    std::size_t capacity() const noexcept { return m_capacity; }

  private:
    struct Cell {
        std::atomic<std::size_t> seq;
        T data;
    };

    std::size_t m_capacity;
    std::size_t m_mask;
    std::unique_ptr<Cell[]> m_cells;
    alignas(64) std::atomic<std::size_t> m_enqueue{0};
    alignas(64) std::atomic<std::size_t> m_dequeue{0};
};

} // namespace plugins

#endif // !_PLUGINS_LOCK_FREE_QUEUE_H_
