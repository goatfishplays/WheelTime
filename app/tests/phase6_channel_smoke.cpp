/**
 * @file phase6_channel_smoke.cpp
 * @brief Disposable smoke / stress tests for ChannelManager (Phase 6).
 */

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/ActionItems.hpp"
#include "App/ChannelManager.hpp"
#include "App/ExecuteResult.hpp"
#include "App/Scheduler.hpp"

#include <atomic>
#include <chrono>
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

bool waitOrderSize(std::mutex &mutex, std::vector<int> &order, std::size_t expected,
                   std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true)
    {
        {
            std::lock_guard lock{mutex};
            if (order.size() >= expected)
            {
                return true;
            }
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
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
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
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
    ScheduleChildItem(std::atomic<int> *childCounter, uint32_t childChannel,
                      std::chrono::steady_clock::time_point wakeTime)
        : m_childCounter{childCounter}
        , m_childChannel{childChannel}
        , m_wakeTime{wakeTime}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<ScheduleChildItem>(m_childCounter, m_childChannel, m_wakeTime);
    }

    ExecuteResult execute(ActionExecutionContext &context) override
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<CountingItem>(m_childCounter));
        auto child = std::make_unique<Action>(std::move(items), "child", "", "child", m_childChannel);
        context.scheduleAction(std::move(child), m_wakeTime);
        return ExecuteResult::Continue();
    }

private:
    std::atomic<int> *m_childCounter = nullptr;
    uint32_t m_childChannel = 0;
    std::chrono::steady_clock::time_point m_wakeTime{};
};

std::unique_ptr<Action> makeAction(std::vector<std::unique_ptr<ActionItem>> items, uint32_t channel)
{
    return std::make_unique<Action>(std::move(items), "smoke", "", "smoke", channel);
}

std::unique_ptr<ActionExecutionContext> makeEmptyContext(uint32_t channel)
{
    auto action = std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "ch", "", "ch", channel);
    return std::make_unique<ActionExecutionContext>(std::move(action));
}

bool testManagerPreservesEarliestStart()
{
    ChannelManager channels;
    const auto now = std::chrono::steady_clock::now();
    const auto wake = now + std::chrono::milliseconds(200);

    auto first = makeEmptyContext(1);
    auto second = makeEmptyContext(1);

    ChannelDispatch firstDispatch = channels.admit(std::move(first), {}, now);
    if (firstDispatch.type != ChannelDispatch::Type::RunNow)
    {
        std::cerr << "manager: expected first RunNow\n";
        return false;
    }

    ChannelDispatch queued = channels.admit(std::move(second), wake, now);
    if (queued.type != ChannelDispatch::Type::None)
    {
        std::cerr << "manager: expected second Queued\n";
        return false;
    }

    ChannelDispatch next = channels.releaseAndTakeNext(1, now);
    if (next.type != ChannelDispatch::Type::ParkUntil || next.wakeTime != wake || !next.context)
    {
        std::cerr << "manager: expected ParkUntil with preserved wake\n";
        return false;
    }
    return true;
}

bool testDeepFifo()
{
    constexpr int count = 20;
    std::mutex mutex;
    std::vector<int> order;

    Scheduler scheduler{4};
    for (int i = 1; i <= count; ++i)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<RecordItem>(&mutex, &order, i));
        scheduler.submit(makeAction(std::move(items), 1));
    }

    if (!waitOrderSize(mutex, order, static_cast<std::size_t>(count), std::chrono::seconds(5)))
    {
        std::cerr << "deep_fifo: timed out\n";
        return false;
    }

    scheduler.stop();

    std::lock_guard lock{mutex};
    if (order.size() != static_cast<std::size_t>(count))
    {
        std::cerr << "deep_fifo: size mismatch\n";
        return false;
    }
    for (int i = 0; i < count; ++i)
    {
        if (order[static_cast<std::size_t>(i)] != i + 1)
        {
            std::cerr << "deep_fifo: order broken at " << i << '\n';
            return false;
        }
    }
    return true;
}

bool testManyParallelChannels()
{
    constexpr int channelCount = 8;
    std::atomic<int> entered{0};
    std::atomic<bool> release{false};

    Scheduler scheduler{8};
    for (int channel = 1; channel <= channelCount; ++channel)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<GateItem>(&entered, &release));
        scheduler.submit(makeAction(std::move(items), static_cast<uint32_t>(channel)));
    }

    if (!waitUntil(entered, channelCount, std::chrono::seconds(2)))
    {
        std::cerr << "many_channels: did not overlap\n";
        return false;
    }

    release.store(true, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    scheduler.stop();
    return true;
}

bool testDelayThenQueued()
{
    std::mutex mutex;
    std::vector<int> order;

    std::vector<std::unique_ptr<ActionItem>> firstItems;
    firstItems.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(100)));
    firstItems.push_back(std::make_unique<RecordItem>(&mutex, &order, 1));

    std::vector<std::unique_ptr<ActionItem>> secondItems;
    secondItems.push_back(std::make_unique<RecordItem>(&mutex, &order, 2));
    secondItems.push_back(std::make_unique<RecordItem>(&mutex, &order, 3));

    Scheduler scheduler{4};
    scheduler.submit(makeAction(std::move(firstItems), 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    scheduler.submit(makeAction(std::move(secondItems), 1));

    if (!waitOrderSize(mutex, order, 3, std::chrono::seconds(3)))
    {
        std::cerr << "delay_queue: timed out\n";
        return false;
    }

    scheduler.stop();

    std::lock_guard lock{mutex};
    if (order != std::vector<int>{1, 2, 3})
    {
        std::cerr << "delay_queue: expected 1,2,3\n";
        return false;
    }
    return true;
}

bool testNestedSameChannelOrder()
{
    std::mutex mutex;
    std::vector<int> order;

    // Parent records 1, then schedules child that records 2 on same channel.
    class ScheduleRecordChild : public ActionItem
    {
    public:
        ScheduleRecordChild(std::mutex *mutex, std::vector<int> *order)
            : m_mutex{mutex}
            , m_order{order}
        {
        }

        std::unique_ptr<ActionItem> clone() const override
        {
            return std::make_unique<ScheduleRecordChild>(m_mutex, m_order);
        }

        ExecuteResult execute(ActionExecutionContext &context) override
        {
            {
                std::lock_guard lock{*m_mutex};
                m_order->push_back(1);
            }
            std::vector<std::unique_ptr<ActionItem>> items;
            items.push_back(std::make_unique<RecordItem>(m_mutex, m_order, 2));
            auto child = std::make_unique<Action>(std::move(items), "child", "", "child", 1);
            context.scheduleAction(std::move(child), std::chrono::steady_clock::now());
            return ExecuteResult::Continue();
        }

    private:
        std::mutex *m_mutex = nullptr;
        std::vector<int> *m_order = nullptr;
    };

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<ScheduleRecordChild>(&mutex, &order));

    Scheduler scheduler{2};
    scheduler.submit(makeAction(std::move(items), 1));

    if (!waitOrderSize(mutex, order, 2, std::chrono::seconds(2)))
    {
        std::cerr << "nested_order: timed out\n";
        return false;
    }

    scheduler.stop();

    std::lock_guard lock{mutex};
    if (order != std::vector<int>{1, 2})
    {
        std::cerr << "nested_order: expected parent then child\n";
        return false;
    }
    return true;
}

bool testNestedFutureWakeWhileBusy()
{
    std::atomic<int> childCounter{0};
    std::atomic<int> blockerDone{0};

    // Blocker holds channel 1.
    std::vector<std::unique_ptr<ActionItem>> blockerItems;
    blockerItems.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(150)));
    blockerItems.push_back(std::make_unique<CountingItem>(&blockerDone));

    // Parent on channel 2 schedules child onto busy channel 1 with a future wake.
    const auto childWake = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
    std::vector<std::unique_ptr<ActionItem>> parentItems;
    parentItems.push_back(std::make_unique<ScheduleChildItem>(&childCounter, 1, childWake));

    Scheduler scheduler{4};
    scheduler.submit(makeAction(std::move(blockerItems), 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    scheduler.submit(makeAction(std::move(parentItems), 2));

    // Child must not run before its wake time.
    std::this_thread::sleep_for(std::chrono::milliseconds(180));
    if (childCounter.load() != 0)
    {
        std::cerr << "future_wake: child ran before wake while channel was busy/queued\n";
        return false;
    }

    if (!waitUntil(childCounter, 1, std::chrono::seconds(2)))
    {
        std::cerr << "future_wake: child never ran\n";
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    if (now + std::chrono::milliseconds(5) < childWake)
    {
        std::cerr << "future_wake: finished before scheduled wake\n";
        return false;
    }

    scheduler.stop();
    return true;
}

bool testChannel0Stress()
{
    constexpr int count = 12;
    std::atomic<int> entered{0};
    std::atomic<bool> release{false};

    // Enough workers that every channel-0 Action can be in-flight together.
    Scheduler scheduler{static_cast<std::size_t>(count)};
    for (int i = 0; i < count; ++i)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<GateItem>(&entered, &release));
        scheduler.submit(makeAction(std::move(items), 0));
    }

    if (!waitUntil(entered, count, std::chrono::seconds(2)))
    {
        std::cerr << "channel0_stress: actions blocked each other\n";
        return false;
    }

    release.store(true, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
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
        {"manager_earliest_start", testManagerPreservesEarliestStart},
        {"deep_fifo", testDeepFifo},
        {"many_parallel_channels", testManyParallelChannels},
        {"delay_then_queued", testDelayThenQueued},
        {"nested_same_channel_order", testNestedSameChannelOrder},
        {"nested_future_wake_busy", testNestedFutureWakeWhileBusy},
        {"channel0_stress", testChannel0Stress},
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

    std::cout << "Phase 6 ChannelManager smoke tests passed\n";
    return 0;
}
