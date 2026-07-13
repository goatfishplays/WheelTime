/**
 * @file phase4_scheduler_smoke.cpp
 * @brief Disposable smoke test for Scheduler (Phase 4).
 */

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/ActionItems.hpp"
#include "App/ExecuteResult.hpp"
#include "App/Scheduler.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace Application;

namespace
{

bool waitUntil(std::atomic<int> &value, int expected, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (value.load(std::memory_order_acquire) < expected)
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return true;
}

class CountingItem : public ActionItem
{
public:
    explicit CountingItem(std::atomic<int> *counter)
        : m_counter{counter}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<CountingItem>(m_counter);
    }

    ExecuteResult execute(ActionExecutionContext & /*context*/) override
    {
        if (m_counter != nullptr)
        {
            m_counter->fetch_add(1, std::memory_order_acq_rel);
        }
        return ExecuteResult::Continue();
    }

private:
    std::atomic<int> *m_counter = nullptr;
};

class DelayItem : public ActionItem
{
public:
    explicit DelayItem(std::chrono::milliseconds delay)
        : m_delay{delay}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<DelayItem>(m_delay);
    }

    ExecuteResult execute(ActionExecutionContext & /*context*/) override
    {
        return ExecuteResult::DelayUntil(std::chrono::steady_clock::now() + m_delay);
    }

private:
    std::chrono::milliseconds m_delay;
};

class RecordItem : public ActionItem
{
public:
    RecordItem(std::mutex *mutex, std::vector<int> *order, int id)
        : m_mutex{mutex}
        , m_order{order}
        , m_id{id}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<RecordItem>(m_mutex, m_order, m_id);
    }

    ExecuteResult execute(ActionExecutionContext & /*context*/) override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        std::lock_guard lock{*m_mutex};
        m_order->push_back(m_id);
        return ExecuteResult::Continue();
    }

private:
    std::mutex *m_mutex = nullptr;
    std::vector<int> *m_order = nullptr;
    int m_id = 0;
};

class GateItem : public ActionItem
{
public:
    GateItem(std::atomic<int> *entered, std::atomic<bool> *release)
        : m_entered{entered}
        , m_release{release}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<GateItem>(m_entered, m_release);
    }

    ExecuteResult execute(ActionExecutionContext & /*context*/) override
    {
        m_entered->fetch_add(1, std::memory_order_acq_rel);
        while (!m_release->load(std::memory_order_acquire))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return ExecuteResult::Continue();
    }

private:
    std::atomic<int> *m_entered = nullptr;
    std::atomic<bool> *m_release = nullptr;
};

class ScheduleChildItem : public ActionItem
{
public:
    explicit ScheduleChildItem(std::atomic<int> *childCounter)
        : m_childCounter{childCounter}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<ScheduleChildItem>(m_childCounter);
    }

    ExecuteResult execute(ActionExecutionContext &context) override
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<CountingItem>(m_childCounter));
        auto child = std::make_unique<Action>(std::move(items), "child", "", "child", 2);
        context.scheduleAction(std::move(child), std::chrono::steady_clock::now());
        return ExecuteResult::Continue();
    }

private:
    std::atomic<int> *m_childCounter = nullptr;
};

std::unique_ptr<Action> makeAction(std::vector<std::unique_ptr<ActionItem>> items, uint32_t channel)
{
    return std::make_unique<Action>(std::move(items), "smoke", "", "smoke", channel);
}

bool testCompleted()
{
    std::atomic<int> counter{0};
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<CountingItem>(&counter));
    items.push_back(std::make_unique<CountingItem>(&counter));

    Scheduler scheduler{2};
    scheduler.submit(makeAction(std::move(items), 1));

    if (!waitUntil(counter, 2, std::chrono::seconds(2)))
    {
        std::cerr << "completed: timed out\n";
        return false;
    }
    scheduler.stop();
    return true;
}

bool testDelayHoldAndResume()
{
    std::atomic<int> counter{0};
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(150)));
    items.push_back(std::make_unique<CountingItem>(&counter));

    Scheduler scheduler{1};
    const auto start = std::chrono::steady_clock::now();
    scheduler.submit(makeAction(std::move(items), 1));

    if (!waitUntil(counter, 1, std::chrono::seconds(2)))
    {
        std::cerr << "delay: timed out waiting for resume\n";
        return false;
    }

    const auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed < std::chrono::milliseconds(120))
    {
        std::cerr << "delay: completed too early\n";
        return false;
    }

    scheduler.stop();
    return true;
}

bool testChannelFifo()
{
    std::mutex mutex;
    std::vector<int> order;

    std::vector<std::unique_ptr<ActionItem>> firstItems;
    firstItems.push_back(std::make_unique<RecordItem>(&mutex, &order, 1));
    std::vector<std::unique_ptr<ActionItem>> secondItems;
    secondItems.push_back(std::make_unique<RecordItem>(&mutex, &order, 2));

    Scheduler scheduler{4};
    scheduler.submit(makeAction(std::move(firstItems), 1));
    scheduler.submit(makeAction(std::move(secondItems), 1));

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (true)
    {
        {
            std::lock_guard lock{mutex};
            if (order.size() == 2)
            {
                break;
            }
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            std::cerr << "fifo: timed out\n";
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    scheduler.stop();

    std::lock_guard lock{mutex};
    if (order.size() != 2 || order[0] != 1 || order[1] != 2)
    {
        std::cerr << "fifo: expected order 1,2\n";
        return false;
    }
    return true;
}

bool testParallelChannels()
{
    std::atomic<int> entered{0};
    std::atomic<bool> release{false};

    std::vector<std::unique_ptr<ActionItem>> aItems;
    aItems.push_back(std::make_unique<GateItem>(&entered, &release));
    std::vector<std::unique_ptr<ActionItem>> bItems;
    bItems.push_back(std::make_unique<GateItem>(&entered, &release));

    Scheduler scheduler{4};
    scheduler.submit(makeAction(std::move(aItems), 1));
    scheduler.submit(makeAction(std::move(bItems), 2));

    if (!waitUntil(entered, 2, std::chrono::seconds(2)))
    {
        std::cerr << "parallel: channels did not overlap\n";
        return false;
    }

    release.store(true, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    scheduler.stop();
    return true;
}

bool testChannel0Parallel()
{
    std::atomic<int> entered{0};
    std::atomic<bool> release{false};

    std::vector<std::unique_ptr<ActionItem>> aItems;
    aItems.push_back(std::make_unique<GateItem>(&entered, &release));
    std::vector<std::unique_ptr<ActionItem>> bItems;
    bItems.push_back(std::make_unique<GateItem>(&entered, &release));

    Scheduler scheduler{4};
    scheduler.submit(makeAction(std::move(aItems), 0));
    scheduler.submit(makeAction(std::move(bItems), 0));

    if (!waitUntil(entered, 2, std::chrono::seconds(2)))
    {
        std::cerr << "channel0: actions blocked each other\n";
        return false;
    }

    release.store(true, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    scheduler.stop();
    return true;
}

bool testDelayHoldsChannel()
{
    std::mutex mutex;
    std::vector<int> order;

    std::vector<std::unique_ptr<ActionItem>> firstItems;
    firstItems.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(120)));
    firstItems.push_back(std::make_unique<RecordItem>(&mutex, &order, 1));

    std::vector<std::unique_ptr<ActionItem>> secondItems;
    secondItems.push_back(std::make_unique<RecordItem>(&mutex, &order, 2));

    Scheduler scheduler{4};
    scheduler.submit(makeAction(std::move(firstItems), 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    scheduler.submit(makeAction(std::move(secondItems), 1));

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (true)
    {
        {
            std::lock_guard lock{mutex};
            if (order.size() == 2)
            {
                break;
            }
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            std::cerr << "hold: timed out\n";
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    scheduler.stop();

    std::lock_guard lock{mutex};
    if (order.size() != 2 || order[0] != 1 || order[1] != 2)
    {
        std::cerr << "hold: expected 1 before 2 while delayed\n";
        return false;
    }
    return true;
}

bool testNestedSchedule()
{
    std::atomic<int> childCounter{0};
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<ScheduleChildItem>(&childCounter));

    Scheduler scheduler{2};
    scheduler.submit(makeAction(std::move(items), 1));

    if (!waitUntil(childCounter, 1, std::chrono::seconds(2)))
    {
        std::cerr << "nested: child did not run\n";
        return false;
    }
    scheduler.stop();
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
        {"completed", testCompleted},
        {"delay_resume", testDelayHoldAndResume},
        {"channel_fifo", testChannelFifo},
        {"parallel_channels", testParallelChannels},
        {"channel0_parallel", testChannel0Parallel},
        {"delay_holds_channel", testDelayHoldsChannel},
        {"nested_schedule", testNestedSchedule},
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

    std::cout << "Phase 4 Scheduler smoke tests passed\n";
    return 0;
}
