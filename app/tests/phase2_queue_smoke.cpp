/**
 * @file phase2_queue_smoke.cpp
 * @brief Disposable smoke test for ThreadSafeQueue (Phase 2).
 */

#include "App/ThreadSafeQueue.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

using Application::ThreadSafeQueue;

namespace
{

bool testMpMcFifo()
{
    constexpr int producerCount = 2;
    constexpr int consumerCount = 2;
    constexpr int perProducer = 100;

    ThreadSafeQueue<int> queue;
    std::atomic<int> produced{0};
    std::mutex resultMutex;
    std::vector<int> received;
    received.reserve(producerCount * perProducer);

    std::vector<std::jthread> producers;
    producers.reserve(producerCount);
    for (int p = 0; p < producerCount; ++p)
    {
        producers.emplace_back([&queue, &produced, p]()
                               {
            for (int i = 0; i < perProducer; ++i)
            {
                const int value = p * perProducer + i;
                queue.push(value);
                produced.fetch_add(1, std::memory_order_relaxed);
            } });
    }

    std::vector<std::jthread> consumers;
    consumers.reserve(consumerCount);
    for (int c = 0; c < consumerCount; ++c)
    {
        consumers.emplace_back([&queue, &resultMutex, &received]()
                               {
            int value = 0;
            while (queue.pop(value))
            {
                std::lock_guard lock{resultMutex};
                received.push_back(value);
            } });
    }

    for (auto &producer : producers)
    {
        producer.join();
    }

    // Wait briefly for the last items to be visible, then stop so consumers exit.
    while (produced.load(std::memory_order_acquire) < producerCount * perProducer)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    queue.stop();

    for (auto &consumer : consumers)
    {
        consumer.join();
    }

    if (static_cast<int>(received.size()) != producerCount * perProducer)
    {
        std::cerr << "MPMC: expected " << (producerCount * perProducer)
                  << " items, got " << received.size() << '\n';
        return false;
    }

    std::set<int> unique(received.begin(), received.end());
    if (unique.size() != received.size())
    {
        std::cerr << "MPMC: duplicate items detected\n";
        return false;
    }

    for (int expected = 0; expected < producerCount * perProducer; ++expected)
    {
        if (!unique.contains(expected))
        {
            std::cerr << "MPMC: missing value " << expected << '\n';
            return false;
        }
    }

    return true;
}

bool testClear()
{
    ThreadSafeQueue<int> queue;
    queue.push(1);
    queue.push(2);
    queue.push(3);
    queue.clear();
    queue.push(42);
    queue.stop();

    int value = 0;
    if (!queue.pop(value) || value != 42)
    {
        std::cerr << "clear: expected to pop 42 after clear\n";
        return false;
    }
    if (queue.pop(value))
    {
        std::cerr << "clear: expected empty after final pop\n";
        return false;
    }
    return true;
}

bool testStopDrainsThenBlocksFalse()
{
    ThreadSafeQueue<int> queue;
    queue.push(7);
    queue.push(8);
    queue.stop();

    int value = 0;
    if (!queue.pop(value) || value != 7)
    {
        std::cerr << "stop/drain: first pop failed\n";
        return false;
    }
    if (!queue.pop(value) || value != 8)
    {
        std::cerr << "stop/drain: second pop failed\n";
        return false;
    }
    if (queue.pop(value))
    {
        std::cerr << "stop/drain: expected false after drain\n";
        return false;
    }

    queue.push(99); // must be ignored after stop
    if (queue.pop(value))
    {
        std::cerr << "stop/drain: push after stop should be ignored\n";
        return false;
    }
    return true;
}

bool testMoveOnlyType()
{
    ThreadSafeQueue<std::unique_ptr<int>> queue;
    queue.push(std::make_unique<int>(123));
    queue.stop();

    std::unique_ptr<int> value;
    if (!queue.pop(value) || value == nullptr || *value != 123)
    {
        std::cerr << "move-only: failed to pop unique_ptr\n";
        return false;
    }
    return true;
}

} // namespace

int main()
{
    struct Case
    {
        const char *name;
        bool (*fn)();
    };

    const Case cases[] = {
        {"mpmc_fifo", testMpMcFifo},
        {"clear", testClear},
        {"stop_drain", testStopDrainsThenBlocksFalse},
        {"move_only", testMoveOnlyType},
    };

    int failed = 0;
    for (const Case &testCase : cases)
    {
        const bool ok = testCase.fn();
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << testCase.name << '\n';
        if (!ok)
        {
            ++failed;
        }
    }

    if (failed != 0)
    {
        std::cerr << failed << " smoke test(s) failed\n";
        return 1;
    }

    std::cout << "Phase 2 ThreadSafeQueue smoke tests passed\n";
    return 0;
}
