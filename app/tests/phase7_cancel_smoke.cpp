/**
 * @file phase7_cancel_smoke.cpp
 * @brief Disposable smoke tests for cancellation (Phase 7).
 */

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/ActionItems.hpp"
#include "App/ExecuteResult.hpp"
#include "App/Scheduler.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
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

class RegisterFlushAndContinueItem : public ActionItem
{
public:
    explicit RegisterFlushAndContinueItem(std::atomic<int> *flushCounter)
        : m_flushCounter{flushCounter}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<RegisterFlushAndContinueItem>(m_flushCounter);
    }

    ExecuteResult execute(ActionExecutionContext &context) override
    {
        std::vector<std::unique_ptr<ActionItem>> flushItems;
        flushItems.push_back(std::make_unique<CountingItem>(m_flushCounter));
        auto flush = std::make_unique<Action>(std::move(flushItems), "flush", "", "flush", 0);
        flush->setCancelable(false);
        context.setCancelFlush(std::move(flush));

        std::vector<std::unique_ptr<ActionItem>> delayedItems;
        delayedItems.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(500)));
        delayedItems.push_back(std::make_unique<CountingItem>(m_flushCounter));
        auto delayed = std::make_unique<Action>(std::move(delayedItems), "delayed", "", "delayed", 0);
        delayed->setCancelable(false);
        context.scheduleAction(
            std::move(delayed),
            std::chrono::steady_clock::now() + std::chrono::milliseconds(500),
            true);

        return ExecuteResult::Continue();
    }

private:
    std::atomic<int> *m_flushCounter = nullptr;
};

std::unique_ptr<Action> makeAction(std::vector<std::unique_ptr<ActionItem>> items, uint32_t channel)
{
    return std::make_unique<Action>(std::move(items), "smoke", "", "smoke", channel);
}

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

    if (second.load() != 0)
    {
        std::cerr << "cancel_queued: queued action still ran\n";
        return false;
    }
    return true;
}

bool testCancelDelayed()
{
    std::atomic<int> afterDelay{0};
    std::atomic<int> follower{0};

    std::vector<std::unique_ptr<ActionItem>> delayedItems;
    delayedItems.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(300)));
    delayedItems.push_back(std::make_unique<CountingItem>(&afterDelay));

    std::vector<std::unique_ptr<ActionItem>> followerItems;
    followerItems.push_back(std::make_unique<CountingItem>(&follower));

    Scheduler scheduler{2};
    const uint64_t delayedId = scheduler.submit(makeAction(std::move(delayedItems), 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    scheduler.submit(makeAction(std::move(followerItems), 1));
    scheduler.cancelAction(delayedId);

    if (!waitUntil(follower, 1, std::chrono::seconds(2)))
    {
        std::cerr << "cancel_delayed: follower never ran\n";
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    scheduler.stop();

    if (afterDelay.load() != 0)
    {
        std::cerr << "cancel_delayed: delayed tail still ran\n";
        return false;
    }
    return true;
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
        std::cerr << "cancel_executing: never entered\n";
        return false;
    }
    scheduler.cancelAction(id);
    release.store(true, std::memory_order_release);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    scheduler.stop();

    if (tail.load() != 0)
    {
        std::cerr << "cancel_executing: tail item still ran\n";
        return false;
    }
    return true;
}

bool testUncancelableSurvivesCancelAll()
{
    std::atomic<int> counter{0};
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(80)));
    items.push_back(std::make_unique<CountingItem>(&counter));
    auto action = makeAction(std::move(items), 0);
    action->setCancelable(false);

    Scheduler scheduler{2};
    scheduler.submit(std::move(action));
    scheduler.cancelAll();

    if (!waitUntil(counter, 1, std::chrono::seconds(2)))
    {
        std::cerr << "uncancelable: cancelled by cancelAll\n";
        return false;
    }
    scheduler.stop();
    return true;
}

bool testCancelFlushRunsImmediately()
{
    std::atomic<int> flushCounter{0};

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<RegisterFlushAndContinueItem>(&flushCounter));
    items.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(1000)));
    items.push_back(std::make_unique<CountingItem>(nullptr));

    Scheduler scheduler{2};
    const uint64_t id = scheduler.submit(makeAction(std::move(items), 1));

    // RegisterFlush yields Continue → scheduler ingests flushes; then Delay parks parent.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    scheduler.cancelAction(id);

    if (!waitUntil(flushCounter, 1, std::chrono::milliseconds(300)))
    {
        std::cerr << "flush: immediate cleanup did not run\n";
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    if (flushCounter.load() != 1)
    {
        std::cerr << "flush: linked delayed cleanup still ran (count=" << flushCounter.load()
                  << ")\n";
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

    // ch1 should be 1 (gate entered) not 2 (tail cancelled)
    if (ch1.load() != 1)
    {
        std::cerr << "cancel_channel: channel 1 tail still ran\n";
        return false;
    }
    return true;
}

bool testCancelUnknownId()
{
    Scheduler scheduler{1};
    scheduler.cancelAction(999999);
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
        {"cancel_queued", testCancelQueued},
        {"cancel_delayed", testCancelDelayed},
        {"cancel_executing", testCancelExecuting},
        {"uncancelable_survives_all", testUncancelableSurvivesCancelAll},
        {"cancel_flush_immediate", testCancelFlushRunsImmediately},
        {"cancel_channel_leaves_others", testCancelChannelLeavesOthers},
        {"cancel_unknown_id", testCancelUnknownId},
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

    std::cout << "Phase 7 cancellation smoke tests passed\n";
    return 0;
}
