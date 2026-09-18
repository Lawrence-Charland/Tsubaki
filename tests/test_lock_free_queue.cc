#include "lock_free_queue.h"
#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

TEST(LockFreeQueue, PushPopFifo) {
    plugins::LockFreeQueue<int> q(4);

    EXPECT_TRUE(q.try_push(1));
    EXPECT_TRUE(q.try_push(2));

    int value = 0;
    EXPECT_TRUE(q.try_pop(value));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(q.try_pop(value));
    EXPECT_EQ(value, 2);
    EXPECT_FALSE(q.try_pop(value));
}

TEST(LockFreeQueue, ReportsFull) {
    plugins::LockFreeQueue<int> q(4);

    EXPECT_TRUE(q.try_push(1));
    EXPECT_TRUE(q.try_push(2));
    EXPECT_TRUE(q.try_push(3));
    EXPECT_TRUE(q.try_push(4));
    EXPECT_FALSE(q.try_push(5));

    int value = 0;
    EXPECT_TRUE(q.try_pop(value));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(q.try_push(5));
    EXPECT_TRUE(q.try_pop(value));
    EXPECT_EQ(value, 2);
}

TEST(LockFreeQueue, ConcurrentProducersAndConsumers) {
    constexpr int kProducers = 4;
    constexpr int kConsumers = 4;
    constexpr int kPerProducer = 5000;
    constexpr int kTotal = kProducers * kPerProducer;

    plugins::LockFreeQueue<int> q(256);
    std::atomic<int> sum{0};
    std::atomic<int> consumed{0};

    std::vector<std::thread> threads;
    for (int p = 0; p < kProducers; ++p) {
        threads.emplace_back([&q] {
            for (int i = 1; i <= kPerProducer; ++i)
                while (!q.try_push(1))
                    std::this_thread::yield();
        });
    }
    for (int c = 0; c < kConsumers; ++c) {
        threads.emplace_back([&q, &sum, &consumed] {
            int value = 0;
            while (consumed.load() < kTotal) {
                if (q.try_pop(value)) {
                    sum.fetch_add(value);
                    consumed.fetch_add(1);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto &t : threads)
        t.join();

    EXPECT_EQ(consumed.load(), kTotal);
    EXPECT_EQ(sum.load(), kTotal);
}
