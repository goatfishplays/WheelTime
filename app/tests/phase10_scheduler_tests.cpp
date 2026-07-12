/**
 * @file phase10_scheduler_tests.cpp
 * @brief Comprehensive scheduler tests (Phase 10).
 *
 * Covers FIFO, channel isolation, delays, cancellation, pause/resume,
 * large queues, multithreaded stress, and randomized scheduling.
 */

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/ActionItems.hpp"
#include "App/ExecuteResult.hpp"
#include "App/Scheduler.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace Application;

namespace
{

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool waitUntil(std::atomic<int> &value, int expected, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (value.load(std::memory_order_acquire) < expected)
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return true;
}

bool waitEquals(std::atomic<int> &value, int expected, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true)
    {
        if (value.load(std::memory_order_acquire) == expected)
        {
            return true;
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
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
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

bool waitPaused(Scheduler &scheduler, bool expected, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (scheduler.isPaused() != expected)
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return true;
}

std::unique_ptr<Action> makeAction(std::vector<std::unique_ptr<ActionItem>> items, uint32_t channel)
{
    return std::make_unique<Action>(std::move(items), "phase10", "", "phase10", channel);
}

// ---------------------------------------------------------------------------
// Test ActionItems
// ---------------------------------------------------------------------------

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

class RecordItem : public ActionItem
{
public:
    RecordItem(std::mutex *mutex, std::vector<int> *order, int id,
               std::chrono::milliseconds spin = std::chrono::milliseconds(1))
        : m_mutex{mutex}
        , m_order{order}
        , m_id{id}
        , m_spin{spin}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<RecordItem>(m_mutex, m_order, m_id, m_spin);
    }

    ExecuteResult execute(ActionExecutionContext & /*context*/) override
    {
        if (m_spin.count() > 0)
        {
            std::this_thread::sleep_for(m_spin);
        }
        std::lock_guard lock{*m_mutex};
        m_order->push_back(m_id);
        return ExecuteResult::Continue();
    }

private:
    std::mutex *m_mutex = nullptr;
    std::vector<int> *m_order = nullptr;
    int m_id = 0;
    std::chrono::milliseconds m_spin;
};

class TimestampItem : public ActionItem
{
public:
    TimestampItem(std::mutex *mutex, std::vector<std::chrono::steady_clock::time_point> *stamps)
        : m_mutex{mutex}
        , m_stamps{stamps}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<TimestampItem>(m_mutex, m_stamps);
    }

    ExecuteResult execute(ActionExecutionContext & /*context*/) override
    {
        const auto now = std::chrono::steady_clock::now();
        std::lock_guard lock{*m_mutex};
        m_stamps->push_back(now);
        return ExecuteResult::Continue();
    }

private:
    std::mutex *m_mutex = nullptr;
    std::vector<std::chrono::steady_clock::time_point> *m_stamps = nullptr;
};

class RegisterFlushItem : public ActionItem
{
public:
    explicit RegisterFlushItem(std::atomic<int> *flushCounter)
        : m_flushCounter{flushCounter}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<RegisterFlushItem>(m_flushCounter);
    }

    ExecuteResult execute(ActionExecutionContext &context) override
    {
        std::vector<std::unique_ptr<ActionItem>> flushItems;
        flushItems.push_back(std::make_unique<CountingItem>(m_flushCounter));
        auto flush = std::make_unique<Action>(std::move(flushItems), "flush", "", "flush", 0);
        flush->setCancelable(false);
        context.setCancelFlush(std::move(flush));
        return ExecuteResult::Continue();
    }

private:
    std::atomic<int> *m_flushCounter = nullptr;
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
        auto child =
            std::make_unique<Action>(std::move(items), "child", "", "child", m_childChannel);
        context.scheduleAction(std::move(child), m_wakeTime);
        return ExecuteResult::Continue();
    }

private:
    std::atomic<int> *m_childCounter = nullptr;
    uint32_t m_childChannel = 0;
    std::chrono::steady_clock::time_point m_wakeTime{};
};

// ---------------------------------------------------------------------------
// FIFO correctness
// ---------------------------------------------------------------------------

bool testFifoDeepQueue()
{
    constexpr int count = 64;
    std::mutex mutex;
    std::vector<int> order;

    Scheduler scheduler{4};
    for (int i = 1; i <= count; ++i)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<RecordItem>(&mutex, &order, i));
        scheduler.submit(makeAction(std::move(items), 1));
    }

    if (!waitOrderSize(mutex, order, static_cast<std::size_t>(count), std::chrono::seconds(10)))
    {
        std::cerr << "fifo_deep: timed out\n";
        return false;
    }
    scheduler.stop();

    std::lock_guard lock{mutex};
    for (int i = 0; i < count; ++i)
    {
        if (order[static_cast<std::size_t>(i)] != i + 1)
        {
            std::cerr << "fifo_deep: order broken at " << i << '\n';
            return false;
        }
    }
    return true;
}

bool testFifoAcrossDelay()
{
    std::mutex mutex;
    std::vector<int> order;

    std::vector<std::unique_ptr<ActionItem>> first;
    first.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(80)));
    first.push_back(std::make_unique<RecordItem>(&mutex, &order, 1));

    std::vector<std::unique_ptr<ActionItem>> second;
    second.push_back(std::make_unique<RecordItem>(&mutex, &order, 2));

    std::vector<std::unique_ptr<ActionItem>> third;
    third.push_back(std::make_unique<RecordItem>(&mutex, &order, 3));

    Scheduler scheduler{4};
    scheduler.submit(makeAction(std::move(first), 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    scheduler.submit(makeAction(std::move(second), 1));
    scheduler.submit(makeAction(std::move(third), 1));

    if (!waitOrderSize(mutex, order, 3, std::chrono::seconds(3)))
    {
        std::cerr << "fifo_delay: timed out\n";
        return false;
    }
    scheduler.stop();

    std::lock_guard lock{mutex};
    if (order != std::vector<int>{1, 2, 3})
    {
        std::cerr << "fifo_delay: expected 1,2,3\n";
        return false;
    }
    return true;
}

bool testFifoMultiItemAction()
{
    std::mutex mutex;
    std::vector<int> order;

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<RecordItem>(&mutex, &order, 1, std::chrono::milliseconds(0)));
    items.push_back(std::make_unique<RecordItem>(&mutex, &order, 2, std::chrono::milliseconds(0)));
    items.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(40)));
    items.push_back(std::make_unique<RecordItem>(&mutex, &order, 3, std::chrono::milliseconds(0)));

    Scheduler scheduler{2};
    scheduler.submit(makeAction(std::move(items), 1));

    if (!waitOrderSize(mutex, order, 3, std::chrono::seconds(2)))
    {
        std::cerr << "fifo_items: timed out\n";
        return false;
    }
    scheduler.stop();

    std::lock_guard lock{mutex};
    if (order != std::vector<int>{1, 2, 3})
    {
        std::cerr << "fifo_items: expected 1,2,3\n";
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Channel isolation
// ---------------------------------------------------------------------------

bool testChannelIsolationParallel()
{
    constexpr int channelCount = 12;
    std::atomic<int> entered{0};
    std::atomic<bool> release{false};

    Scheduler scheduler{static_cast<std::size_t>(channelCount)};
    for (int channel = 1; channel <= channelCount; ++channel)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<GateItem>(&entered, &release));
        scheduler.submit(makeAction(std::move(items), static_cast<uint32_t>(channel)));
    }

    if (!waitUntil(entered, channelCount, std::chrono::seconds(3)))
    {
        std::cerr << "channel_parallel: channels did not overlap (entered=" << entered.load()
                  << ")\n";
        return false;
    }

    release.store(true, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    scheduler.stop();
    return true;
}

bool testChannel0DoesNotSerialize()
{
    constexpr int count = 16;
    std::atomic<int> entered{0};
    std::atomic<bool> release{false};

    Scheduler scheduler{static_cast<std::size_t>(count)};
    for (int i = 0; i < count; ++i)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<GateItem>(&entered, &release));
        scheduler.submit(makeAction(std::move(items), 0));
    }

    if (!waitUntil(entered, count, std::chrono::seconds(3)))
    {
        std::cerr << "channel0: actions blocked each other (entered=" << entered.load() << ")\n";
        return false;
    }

    release.store(true, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    scheduler.stop();
    return true;
}

bool testSameChannelSerializes()
{
    std::atomic<int> concurrent{0};
    std::atomic<int> maxConcurrent{0};
    std::atomic<int> done{0};
    std::atomic<bool> release{false};

    class ConcurrencyProbe : public ActionItem
    {
    public:
        ConcurrencyProbe(std::atomic<int> *concurrent, std::atomic<int> *maxConcurrent,
                         std::atomic<bool> *release)
            : m_concurrent{concurrent}
            , m_maxConcurrent{maxConcurrent}
            , m_release{release}
        {
        }

        std::unique_ptr<ActionItem> clone() const override
        {
            return std::make_unique<ConcurrencyProbe>(m_concurrent, m_maxConcurrent, m_release);
        }

        ExecuteResult execute(ActionExecutionContext & /*context*/) override
        {
            const int now = m_concurrent->fetch_add(1, std::memory_order_acq_rel) + 1;
            int prev = m_maxConcurrent->load(std::memory_order_acquire);
            while (now > prev &&
                   !m_maxConcurrent->compare_exchange_weak(prev, now, std::memory_order_acq_rel))
            {
            }
            while (!m_release->load(std::memory_order_acquire))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            m_concurrent->fetch_sub(1, std::memory_order_acq_rel);
            return ExecuteResult::Continue();
        }

    private:
        std::atomic<int> *m_concurrent = nullptr;
        std::atomic<int> *m_maxConcurrent = nullptr;
        std::atomic<bool> *m_release = nullptr;
    };

    Scheduler scheduler{8};
    for (int i = 0; i < 6; ++i)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<ConcurrencyProbe>(&concurrent, &maxConcurrent, &release));
        items.push_back(std::make_unique<CountingItem>(&done));
        scheduler.submit(makeAction(std::move(items), 7));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    if (maxConcurrent.load() != 1)
    {
        std::cerr << "same_channel: expected max concurrency 1, got " << maxConcurrent.load()
                  << '\n';
        release.store(true, std::memory_order_release);
        scheduler.stop();
        return false;
    }

    release.store(true, std::memory_order_release);
    if (!waitUntil(done, 6, std::chrono::seconds(3)))
    {
        std::cerr << "same_channel: not all actions finished\n";
        scheduler.stop();
        return false;
    }
    scheduler.stop();
    return true;
}

// ---------------------------------------------------------------------------
// Delay correctness
// ---------------------------------------------------------------------------

bool testDelayWakeTime()
{
    std::mutex mutex;
    std::vector<std::chrono::steady_clock::time_point> stamps;
    constexpr auto delay = std::chrono::milliseconds(120);

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<DelayItem>(delay));
    items.push_back(std::make_unique<TimestampItem>(&mutex, &stamps));

    Scheduler scheduler{2};
    const auto submittedAt = std::chrono::steady_clock::now();
    scheduler.submit(makeAction(std::move(items), 1));

    const auto stampDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (true)
    {
        {
            std::lock_guard lock{mutex};
            if (!stamps.empty())
            {
                break;
            }
        }
        if (std::chrono::steady_clock::now() >= stampDeadline)
        {
            std::cerr << "delay_wake: timed out\n";
            scheduler.stop();
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    scheduler.stop();

    std::lock_guard lock{mutex};
    const auto elapsed = stamps.front() - submittedAt;
    if (elapsed < delay - std::chrono::milliseconds(20))
    {
        std::cerr << "delay_wake: completed too early\n";
        return false;
    }
    return true;
}

bool testDelayHoldsChannel()
{
    std::mutex mutex;
    std::vector<int> order;

    std::vector<std::unique_ptr<ActionItem>> delayed;
    delayed.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(100)));
    delayed.push_back(std::make_unique<RecordItem>(&mutex, &order, 1));

    std::vector<std::unique_ptr<ActionItem>> follower;
    follower.push_back(std::make_unique<RecordItem>(&mutex, &order, 2));

    Scheduler scheduler{4};
    scheduler.submit(makeAction(std::move(delayed), 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    scheduler.submit(makeAction(std::move(follower), 1));

    // Follower must not run during the hold.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    {
        std::lock_guard lock{mutex};
        if (!order.empty())
        {
            std::cerr << "delay_hold: follower or delayed ran early\n";
            scheduler.stop();
            return false;
        }
    }

    if (!waitOrderSize(mutex, order, 2, std::chrono::seconds(3)))
    {
        std::cerr << "delay_hold: timed out\n";
        return false;
    }
    scheduler.stop();

    std::lock_guard lock{mutex};
    if (order != std::vector<int>{1, 2})
    {
        std::cerr << "delay_hold: expected 1 then 2\n";
        return false;
    }
    return true;
}

bool testMultipleIndependentDelays()
{
    std::atomic<int> done{0};
    Scheduler scheduler{4};

    for (int channel = 1; channel <= 4; ++channel)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(50 + channel * 10)));
        items.push_back(std::make_unique<CountingItem>(&done));
        scheduler.submit(makeAction(std::move(items), static_cast<uint32_t>(channel)));
    }

    if (!waitUntil(done, 4, std::chrono::seconds(3)))
    {
        std::cerr << "multi_delay: not all completed (done=" << done.load() << ")\n";
        scheduler.stop();
        return false;
    }
    scheduler.stop();
    return true;
}

bool testNestedFutureWakePreserved()
{
    std::atomic<int> childCounter{0};

    std::vector<std::unique_ptr<ActionItem>> blocker;
    blocker.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(120)));
    blocker.push_back(std::make_unique<CountingItem>(nullptr));

    const auto childWake = std::chrono::steady_clock::now() + std::chrono::milliseconds(220);
    std::vector<std::unique_ptr<ActionItem>> parent;
    parent.push_back(std::make_unique<ScheduleChildItem>(&childCounter, 1, childWake));

    Scheduler scheduler{4};
    scheduler.submit(makeAction(std::move(blocker), 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    scheduler.submit(makeAction(std::move(parent), 2));

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    if (childCounter.load() != 0)
    {
        std::cerr << "nested_wake: child ran before wake\n";
        scheduler.stop();
        return false;
    }

    if (!waitUntil(childCounter, 1, std::chrono::seconds(3)))
    {
        std::cerr << "nested_wake: child never ran\n";
        scheduler.stop();
        return false;
    }

    if (std::chrono::steady_clock::now() + std::chrono::milliseconds(5) < childWake)
    {
        std::cerr << "nested_wake: finished before scheduled wake\n";
        scheduler.stop();
        return false;
    }

    scheduler.stop();
    return true;
}

// ---------------------------------------------------------------------------
// Cancellation correctness
// ---------------------------------------------------------------------------

bool testCancelQueued()
{
    std::atomic<int> first{0};
    std::atomic<int> second{0};
    std::atomic<bool> release{false};

    std::vector<std::unique_ptr<ActionItem>> aItems;
    aItems.push_back(std::make_unique<GateItem>(&first, &release));
    std::vector<std::unique_ptr<ActionItem>> bItems;
    bItems.push_back(std::make_unique<CountingItem>(&second));

    Scheduler scheduler{2};
    scheduler.submit(makeAction(std::move(aItems), 1));
    if (!waitUntil(first, 1, std::chrono::seconds(2)))
    {
        std::cerr << "cancel_queued: first never entered\n";
        return false;
    }
    const uint64_t queuedId = scheduler.submit(makeAction(std::move(bItems), 1));
    scheduler.cancelAction(queuedId);
    release.store(true, std::memory_order_release);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    scheduler.stop();
    return second.load() == 0;
}

bool testCancelDelayed()
{
    std::atomic<int> afterDelay{0};
    std::atomic<int> follower{0};

    std::vector<std::unique_ptr<ActionItem>> delayedItems;
    delayedItems.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(250)));
    delayedItems.push_back(std::make_unique<CountingItem>(&afterDelay));

    std::vector<std::unique_ptr<ActionItem>> followerItems;
    followerItems.push_back(std::make_unique<CountingItem>(&follower));

    Scheduler scheduler{2};
    const uint64_t delayedId = scheduler.submit(makeAction(std::move(delayedItems), 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    scheduler.submit(makeAction(std::move(followerItems), 1));
    scheduler.cancelAction(delayedId);

    if (!waitUntil(follower, 1, std::chrono::seconds(2)))
    {
        std::cerr << "cancel_delayed: follower never ran\n";
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    scheduler.stop();
    return afterDelay.load() == 0;
}

bool testCancelExecuting()
{
    std::atomic<int> mid{0};
    std::atomic<int> tail{0};
    std::atomic<bool> release{false};

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<GateItem>(&mid, &release));
    items.push_back(std::make_unique<CountingItem>(&tail));

    Scheduler scheduler{1};
    const uint64_t id = scheduler.submit(makeAction(std::move(items), 1));
    if (!waitUntil(mid, 1, std::chrono::seconds(2)))
    {
        return false;
    }
    scheduler.cancelAction(id);
    release.store(true, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    scheduler.stop();
    return tail.load() == 0;
}

bool testCancelFlushImmediate()
{
    std::atomic<int> flushCounter{0};
    std::atomic<int> entered{0};
    std::atomic<bool> release{false};

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<RegisterFlushItem>(&flushCounter));
    items.push_back(std::make_unique<GateItem>(&entered, &release));
    items.push_back(std::make_unique<CountingItem>(nullptr));

    Scheduler scheduler{2};
    const uint64_t id = scheduler.submit(makeAction(std::move(items), 1));
    if (!waitUntil(entered, 1, std::chrono::seconds(2)))
    {
        return false;
    }
    scheduler.cancelAction(id);
    if (!waitUntil(flushCounter, 1, std::chrono::seconds(2)))
    {
        std::cerr << "cancel_flush: flush did not run\n";
        release.store(true, std::memory_order_release);
        scheduler.stop();
        return false;
    }
    release.store(true, std::memory_order_release);
    scheduler.stop();
    return true;
}

bool testUncancelableSurvives()
{
    std::atomic<int> counter{0};
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(60)));
    items.push_back(std::make_unique<CountingItem>(&counter));
    auto action = makeAction(std::move(items), 0);
    action->setCancelable(false);

    Scheduler scheduler{2};
    scheduler.submit(std::move(action));
    scheduler.cancelAll();

    if (!waitUntil(counter, 1, std::chrono::seconds(2)))
    {
        std::cerr << "uncancelable: cancelled by cancelAll\n";
        scheduler.stop();
        return false;
    }
    scheduler.stop();
    return true;
}

bool testCancelChannelLeavesOthers()
{
    std::atomic<int> ch1{0};
    std::atomic<int> ch2{0};
    std::atomic<bool> release{false};

    std::vector<std::unique_ptr<ActionItem>> aItems;
    aItems.push_back(std::make_unique<GateItem>(&ch1, &release));
    aItems.push_back(std::make_unique<CountingItem>(&ch1));

    std::vector<std::unique_ptr<ActionItem>> bItems;
    bItems.push_back(std::make_unique<CountingItem>(&ch2));

    Scheduler scheduler{4};
    scheduler.submit(makeAction(std::move(aItems), 1));
    if (!waitUntil(ch1, 1, std::chrono::seconds(2)))
    {
        return false;
    }
    scheduler.submit(makeAction(std::move(bItems), 2));
    scheduler.cancelChannel(1);
    release.store(true, std::memory_order_release);

    if (!waitUntil(ch2, 1, std::chrono::seconds(2)))
    {
        std::cerr << "cancel_channel: channel 2 blocked\n";
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    scheduler.stop();
    return ch1.load() == 1;
}

// ---------------------------------------------------------------------------
// Pause / resume
// ---------------------------------------------------------------------------

bool testPauseStopsDispatch()
{
    std::atomic<int> count{0};
    std::atomic<int> entered{0};
    std::atomic<bool> release{false};

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<GateItem>(&entered, &release));
    items.push_back(std::make_unique<CountingItem>(&count));

    Scheduler scheduler{2};
    scheduler.submit(makeAction(std::move(items), 1));
    if (!waitUntil(entered, 1, std::chrono::seconds(2)))
    {
        return false;
    }

    scheduler.pause();
    if (!waitPaused(scheduler, true, std::chrono::seconds(2)))
    {
        return false;
    }

    release.store(true, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    if (count.load() != 0)
    {
        std::cerr << "pause_dispatch: second item ran while paused\n";
        scheduler.resume();
        scheduler.stop();
        return false;
    }

    scheduler.resume();
    if (!waitUntil(count, 1, std::chrono::seconds(2)))
    {
        std::cerr << "pause_dispatch: did not resume\n";
        scheduler.stop();
        return false;
    }
    scheduler.stop();
    return true;
}

bool testSubmitWhilePaused()
{
    std::atomic<int> count{0};
    Scheduler scheduler{2};
    scheduler.pause();
    if (!waitPaused(scheduler, true, std::chrono::seconds(2)))
    {
        return false;
    }

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<CountingItem>(&count));
    scheduler.submit(makeAction(std::move(items), 1));

    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    if (count.load() != 0)
    {
        std::cerr << "submit_paused: ran before resume\n";
        scheduler.resume();
        scheduler.stop();
        return false;
    }

    scheduler.resume();
    if (!waitUntil(count, 1, std::chrono::seconds(2)))
    {
        scheduler.stop();
        return false;
    }
    scheduler.stop();
    return true;
}

bool testPauseFreezesDelay()
{
    std::atomic<int> count{0};
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(280)));
    items.push_back(std::make_unique<CountingItem>(&count));

    Scheduler scheduler{2};
    scheduler.submit(makeAction(std::move(items), 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    scheduler.pause();

    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    if (count.load() != 0)
    {
        std::cerr << "freeze_delay: completed while paused\n";
        scheduler.resume();
        scheduler.stop();
        return false;
    }

    const auto resumedAt = std::chrono::steady_clock::now();
    scheduler.resume();
    if (!waitUntil(count, 1, std::chrono::seconds(2)))
    {
        scheduler.stop();
        return false;
    }
    if (std::chrono::steady_clock::now() - resumedAt < std::chrono::milliseconds(80))
    {
        std::cerr << "freeze_delay: completed too fast after resume\n";
        scheduler.stop();
        return false;
    }
    scheduler.stop();
    return true;
}

bool testCancelFlushWhilePaused()
{
    std::atomic<int> entered{0};
    std::atomic<bool> release{false};
    std::atomic<int> flushCount{0};

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<RegisterFlushItem>(&flushCount));
    items.push_back(std::make_unique<GateItem>(&entered, &release));

    Scheduler scheduler{2};
    const uint64_t id = scheduler.submit(makeAction(std::move(items), 1));
    if (!waitUntil(entered, 1, std::chrono::seconds(2)))
    {
        return false;
    }

    scheduler.pause();
    if (!waitPaused(scheduler, true, std::chrono::seconds(2)))
    {
        release.store(true, std::memory_order_release);
        return false;
    }
    scheduler.cancelAction(id);

    if (!waitUntil(flushCount, 1, std::chrono::seconds(2)))
    {
        std::cerr << "flush_paused: cancel-flush did not run\n";
        release.store(true, std::memory_order_release);
        scheduler.resume();
        scheduler.stop();
        return false;
    }

    release.store(true, std::memory_order_release);
    scheduler.resume();
    scheduler.stop();
    return true;
}

// ---------------------------------------------------------------------------
// Large queues
// ---------------------------------------------------------------------------

bool testLargeSingleChannelQueue()
{
    constexpr int count = 400;
    std::atomic<int> done{0};

    Scheduler scheduler{4};
    for (int i = 0; i < count; ++i)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<CountingItem>(&done));
        scheduler.submit(makeAction(std::move(items), 1));
    }

    if (!waitUntil(done, count, std::chrono::seconds(30)))
    {
        std::cerr << "large_queue: done=" << done.load() << " expected=" << count << '\n';
        scheduler.stop();
        return false;
    }
    scheduler.stop();
    return true;
}

bool testLargeMultiChannelQueue()
{
    constexpr int channels = 16;
    constexpr int perChannel = 40;
    std::atomic<int> done{0};

    Scheduler scheduler{8};
    for (int i = 0; i < perChannel; ++i)
    {
        for (int channel = 1; channel <= channels; ++channel)
        {
            std::vector<std::unique_ptr<ActionItem>> items;
            items.push_back(std::make_unique<CountingItem>(&done));
            scheduler.submit(makeAction(std::move(items), static_cast<uint32_t>(channel)));
        }
    }

    const int expected = channels * perChannel;
    if (!waitUntil(done, expected, std::chrono::seconds(30)))
    {
        std::cerr << "large_multi: done=" << done.load() << " expected=" << expected << '\n';
        scheduler.stop();
        return false;
    }
    scheduler.stop();
    return true;
}

bool testLargeFifoOrderPreserved()
{
    constexpr int count = 200;
    std::mutex mutex;
    std::vector<int> order;

    Scheduler scheduler{2};
    for (int i = 1; i <= count; ++i)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(
            std::make_unique<RecordItem>(&mutex, &order, i, std::chrono::milliseconds(0)));
        scheduler.submit(makeAction(std::move(items), 3));
    }

    if (!waitOrderSize(mutex, order, static_cast<std::size_t>(count), std::chrono::seconds(30)))
    {
        std::cerr << "large_fifo: timed out size=" << order.size() << '\n';
        scheduler.stop();
        return false;
    }
    scheduler.stop();

    std::lock_guard lock{mutex};
    for (int i = 0; i < count; ++i)
    {
        if (order[static_cast<std::size_t>(i)] != i + 1)
        {
            std::cerr << "large_fifo: order broken at " << i << '\n';
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Heavy multithreaded stress
// ---------------------------------------------------------------------------

bool testMultithreadedSubmitStress()
{
    constexpr int submitters = 8;
    constexpr int perSubmitter = 50;
    std::atomic<int> done{0};

    Scheduler scheduler{8};
    std::vector<std::thread> threads;
    threads.reserve(submitters);

    for (int t = 0; t < submitters; ++t)
    {
        threads.emplace_back([&, t]() {
            const uint32_t channel = static_cast<uint32_t>((t % 8) + 1);
            for (int i = 0; i < perSubmitter; ++i)
            {
                std::vector<std::unique_ptr<ActionItem>> items;
                items.push_back(std::make_unique<CountingItem>(&done));
                if ((i % 7) == 0)
                {
                    items.insert(items.begin(),
                                 std::make_unique<DelayItem>(std::chrono::milliseconds(5)));
                }
                scheduler.submit(makeAction(std::move(items), channel));
            }
        });
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    const int expected = submitters * perSubmitter;
    if (!waitUntil(done, expected, std::chrono::seconds(30)))
    {
        std::cerr << "mt_submit: done=" << done.load() << " expected=" << expected << '\n';
        scheduler.stop();
        return false;
    }
    scheduler.stop();
    return true;
}

bool testMultithreadedCancelStress()
{
    constexpr int actions = 120;
    std::atomic<int> started{0};
    std::atomic<int> completed{0};
    std::atomic<bool> release{false};

    Scheduler scheduler{6};
    std::vector<uint64_t> ids;
    ids.reserve(actions);

    for (int i = 0; i < actions; ++i)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<GateItem>(&started, &release));
        items.push_back(std::make_unique<CountingItem>(&completed));
        ids.push_back(scheduler.submit(makeAction(std::move(items), static_cast<uint32_t>((i % 6) + 1))));
    }

    // Cancel roughly half from multiple threads while gates hold.
    std::vector<std::thread> cancelThreads;
    for (int t = 0; t < 4; ++t)
    {
        cancelThreads.emplace_back([&, t]() {
            for (std::size_t i = static_cast<std::size_t>(t); i < ids.size(); i += 4)
            {
                if ((i % 2) == 0)
                {
                    scheduler.cancelAction(ids[i]);
                }
            }
        });
    }
    for (auto &thread : cancelThreads)
    {
        thread.join();
    }

    release.store(true, std::memory_order_release);

    // Odd-indexed actions should complete; even were cancelled (best-effort —
    // some may have finished gate+tail before cancel). Wait until activity settles.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    int last = -1;
    int stable = 0;
    while (std::chrono::steady_clock::now() < deadline)
    {
        const int cur = completed.load(std::memory_order_acquire);
        if (cur == last)
        {
            ++stable;
            if (stable > 20)
            {
                break;
            }
        }
        else
        {
            stable = 0;
            last = cur;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    scheduler.stop();

    // Must not overshoot total actions; cancelled ones should keep count well below total.
    if (completed.load() > actions)
    {
        std::cerr << "mt_cancel: completed exceeds submitted\n";
        return false;
    }
    if (completed.load() == actions)
    {
        std::cerr << "mt_cancel: expected some cancellations to stick\n";
        return false;
    }
    return true;
}

bool testPauseResumeUnderLoad()
{
    constexpr int count = 80;
    std::atomic<int> done{0};

    Scheduler scheduler{6};
    for (int i = 0; i < count; ++i)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(10)));
        items.push_back(std::make_unique<CountingItem>(&done));
        scheduler.submit(makeAction(std::move(items), static_cast<uint32_t>((i % 4) + 1)));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    scheduler.pause();
    if (!waitPaused(scheduler, true, std::chrono::seconds(2)))
    {
        scheduler.resume();
        scheduler.stop();
        return false;
    }

    const int atPause = done.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // Allow items already on workers to finish; then progress should stop.
    const int midPause = done.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    if (done.load() > midPause)
    {
        std::cerr << "pause_load: progress continued while paused (" << midPause << " -> "
                  << done.load() << ", atPause=" << atPause << ")\n";
        scheduler.resume();
        scheduler.stop();
        return false;
    }

    scheduler.resume();
    if (!waitUntil(done, count, std::chrono::seconds(15)))
    {
        std::cerr << "pause_load: did not finish after resume done=" << done.load() << '\n';
        scheduler.stop();
        return false;
    }
    scheduler.stop();
    return true;
}

// ---------------------------------------------------------------------------
// Randomized scheduling
// ---------------------------------------------------------------------------

bool testRandomizedScheduling()
{
    constexpr int iterations = 300;
    std::atomic<int> completed{0};
    std::mt19937 rng{0xC0FFEEu};
    std::uniform_int_distribution<int> opDist{0, 99};
    std::uniform_int_distribution<int> channelDist{0, 8};
    std::uniform_int_distribution<int> delayDist{0, 40};

    Scheduler scheduler{6};
    std::vector<uint64_t> liveIds;
    liveIds.reserve(iterations);
    std::mutex idMutex;
    int submitted = 0;

    for (int i = 0; i < iterations; ++i)
    {
        const int op = opDist(rng);

        if (op < 55)
        {
            // Submit
            const uint32_t channel = static_cast<uint32_t>(channelDist(rng));
            std::vector<std::unique_ptr<ActionItem>> items;
            const int delayMs = delayDist(rng);
            if (delayMs > 0)
            {
                items.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(delayMs)));
            }
            items.push_back(std::make_unique<CountingItem>(&completed));
            auto action = makeAction(std::move(items), channel);
            if (op < 5)
            {
                action->setCancelable(false);
            }
            const uint64_t id = scheduler.submit(std::move(action));
            {
                std::lock_guard lock{idMutex};
                liveIds.push_back(id);
            }
            ++submitted;
        }
        else if (op < 70)
        {
            // Cancel one
            uint64_t id = 0;
            {
                std::lock_guard lock{idMutex};
                if (!liveIds.empty())
                {
                    std::uniform_int_distribution<std::size_t> pick{0, liveIds.size() - 1};
                    id = liveIds[pick(rng)];
                }
            }
            if (id != 0)
            {
                scheduler.cancelAction(id);
            }
        }
        else if (op < 80)
        {
            scheduler.cancelChannel(static_cast<uint32_t>(channelDist(rng) % 8 + 1));
        }
        else if (op < 88)
        {
            scheduler.pause();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            scheduler.resume();
        }
        else if (op < 94)
        {
            scheduler.cancelAll();
        }
        else
        {
            // Burst submit on channel 0
            for (int b = 0; b < 3; ++b)
            {
                std::vector<std::unique_ptr<ActionItem>> items;
                items.push_back(std::make_unique<CountingItem>(&completed));
                scheduler.submit(makeAction(std::move(items), 0));
                ++submitted;
            }
        }

        if ((i % 40) == 0)
        {
            std::this_thread::yield();
        }
    }

    // Drain remaining work without further cancels.
    scheduler.resume();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    int last = -1;
    int stableRounds = 0;
    while (std::chrono::steady_clock::now() < deadline)
    {
        const int cur = completed.load(std::memory_order_acquire);
        if (cur == last)
        {
            ++stableRounds;
            if (stableRounds > 40)
            {
                break;
            }
        }
        else
        {
            stableRounds = 0;
            last = cur;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    scheduler.stop();

    if (completed.load() > submitted)
    {
        std::cerr << "random: completed (" << completed.load() << ") > submitted (" << submitted
                  << ")\n";
        return false;
    }
    if (completed.load() == 0 && submitted > 0)
    {
        std::cerr << "random: nothing completed\n";
        return false;
    }

    std::cout << "  random: submitted=" << submitted << " completed=" << completed.load() << '\n';
    return true;
}

bool testRandomizedFifoInvariant()
{
    // While random noise runs on other channels, channel 1 must stay FIFO.
    constexpr int fifoCount = 80;
    std::mutex mutex;
    std::vector<int> order;
    std::atomic<int> noiseDone{0};
    std::atomic<bool> stopNoise{false};

    Scheduler scheduler{8};

    std::thread noise{[&]() {
        std::mt19937 rng{42};
        std::uniform_int_distribution<int> channelDist{2, 10};
        std::uniform_int_distribution<int> delayDist{0, 15};
        while (!stopNoise.load(std::memory_order_acquire))
        {
            std::vector<std::unique_ptr<ActionItem>> items;
            const int delayMs = delayDist(rng);
            if (delayMs > 0)
            {
                items.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(delayMs)));
            }
            items.push_back(std::make_unique<CountingItem>(&noiseDone));
            scheduler.submit(makeAction(std::move(items), static_cast<uint32_t>(channelDist(rng))));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }};

    for (int i = 1; i <= fifoCount; ++i)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(
            std::make_unique<RecordItem>(&mutex, &order, i, std::chrono::milliseconds(0)));
        scheduler.submit(makeAction(std::move(items), 1));
    }

    if (!waitOrderSize(mutex, order, static_cast<std::size_t>(fifoCount), std::chrono::seconds(20)))
    {
        stopNoise.store(true, std::memory_order_release);
        noise.join();
        scheduler.stop();
        std::cerr << "random_fifo: timed out\n";
        return false;
    }

    stopNoise.store(true, std::memory_order_release);
    noise.join();
    scheduler.stop();

    std::lock_guard lock{mutex};
    for (int i = 0; i < fifoCount; ++i)
    {
        if (order[static_cast<std::size_t>(i)] != i + 1)
        {
            std::cerr << "random_fifo: order broken at " << i << '\n';
            return false;
        }
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
        // FIFO
        {"fifo_deep_queue", testFifoDeepQueue},
        {"fifo_across_delay", testFifoAcrossDelay},
        {"fifo_multi_item_action", testFifoMultiItemAction},
        // Channel isolation
        {"channel_isolation_parallel", testChannelIsolationParallel},
        {"channel0_does_not_serialize", testChannel0DoesNotSerialize},
        {"same_channel_serializes", testSameChannelSerializes},
        // Delay
        {"delay_wake_time", testDelayWakeTime},
        {"delay_holds_channel", testDelayHoldsChannel},
        {"multiple_independent_delays", testMultipleIndependentDelays},
        {"nested_future_wake_preserved", testNestedFutureWakePreserved},
        // Cancellation
        {"cancel_queued", testCancelQueued},
        {"cancel_delayed", testCancelDelayed},
        {"cancel_executing", testCancelExecuting},
        {"cancel_flush_immediate", testCancelFlushImmediate},
        {"uncancelable_survives", testUncancelableSurvives},
        {"cancel_channel_leaves_others", testCancelChannelLeavesOthers},
        // Pause / resume
        {"pause_stops_dispatch", testPauseStopsDispatch},
        {"submit_while_paused", testSubmitWhilePaused},
        {"pause_freezes_delay", testPauseFreezesDelay},
        {"cancel_flush_while_paused", testCancelFlushWhilePaused},
        // Large queues
        {"large_single_channel_queue", testLargeSingleChannelQueue},
        {"large_multi_channel_queue", testLargeMultiChannelQueue},
        {"large_fifo_order_preserved", testLargeFifoOrderPreserved},
        // Stress
        {"multithreaded_submit_stress", testMultithreadedSubmitStress},
        {"multithreaded_cancel_stress", testMultithreadedCancelStress},
        {"pause_resume_under_load", testPauseResumeUnderLoad},
        // Randomized
        {"randomized_scheduling", testRandomizedScheduling},
        {"randomized_fifo_invariant", testRandomizedFifoInvariant},
    };

    int failed = 0;
    for (const Case &testCase : cases)
    {
        std::cout << "[RUN ] " << testCase.name << '\n';
        const bool ok = testCase.fn();
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << testCase.name << '\n';
        if (!ok)
        {
            ++failed;
        }
    }

    if (failed != 0)
    {
        std::cerr << failed << " phase10 test(s) failed\n";
        return 1;
    }

    std::cout << "Phase 10 scheduler tests passed (" << std::size(cases) << " cases)\n";
    return 0;
}
