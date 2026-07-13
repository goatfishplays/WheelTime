/**
 * @file phase8_pause_smoke.cpp
 * @brief Disposable smoke tests for pause/resume (Phase 8).
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

std::unique_ptr<Action> makeAction(std::vector<std::unique_ptr<ActionItem>> items, uint32_t channel)
{
    return std::make_unique<Action>(std::move(items), "smoke", "", "smoke", channel);
}

bool testPauseStopsNextItem()
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
        std::cerr << "pause_next: never entered gate\n";
        return false;
    }

    scheduler.pause();
    const auto pauseDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!scheduler.isPaused())
    {
        if (std::chrono::steady_clock::now() >= pauseDeadline)
        {
            std::cerr << "pause_next: pause never took effect\n";
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    release.store(true, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    if (count.load(std::memory_order_acquire) != 0)
    {
        std::cerr << "pause_next: second item ran while paused\n";
        return false;
    }

    scheduler.resume();
    if (!waitUntil(count, 1, std::chrono::seconds(2)))
    {
        std::cerr << "pause_next: second item did not run after resume\n";
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
    // Give pause time to take effect on the scheduler thread.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<CountingItem>(&count));
    scheduler.submit(makeAction(std::move(items), 1));

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    if (count.load(std::memory_order_acquire) != 0)
    {
        std::cerr << "submit_paused: action ran before resume\n";
        return false;
    }

    scheduler.resume();
    if (!waitUntil(count, 1, std::chrono::seconds(2)))
    {
        std::cerr << "submit_paused: action did not run after resume\n";
        return false;
    }

    scheduler.stop();
    return true;
}

bool testPauseFreezesDelay()
{
    std::atomic<int> count{0};

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(300)));
    items.push_back(std::make_unique<CountingItem>(&count));

    Scheduler scheduler{2};
    scheduler.submit(makeAction(std::move(items), 1));
    // Let the delay item park on DelayQueue.
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    scheduler.pause();

    // Wall-clock wait longer than the delay; frozen clock should keep it pending.
    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    if (count.load(std::memory_order_acquire) != 0)
    {
        std::cerr << "freeze_delay: delay completed while paused\n";
        return false;
    }

    const auto resumedAt = std::chrono::steady_clock::now();
    scheduler.resume();
    if (!waitUntil(count, 1, std::chrono::seconds(2)))
    {
        std::cerr << "freeze_delay: did not complete after resume\n";
        return false;
    }

    const auto elapsed = std::chrono::steady_clock::now() - resumedAt;
    // Remaining delay should still be meaningful (not instant overdue dump).
    if (elapsed < std::chrono::milliseconds(100))
    {
        std::cerr << "freeze_delay: completed too fast after resume (delay not frozen)\n";
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
        std::cerr << "flush_paused: never entered gate\n";
        return false;
    }

    scheduler.pause();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    scheduler.cancelAction(id);

    if (!waitUntil(flushCount, 1, std::chrono::seconds(2)))
    {
        std::cerr << "flush_paused: cancel-flush did not run while paused\n";
        return false;
    }

    release.store(true, std::memory_order_release);
    scheduler.resume();
    scheduler.stop();
    return true;
}

bool testCancelQueuedWhilePaused()
{
    std::atomic<int> count{0};

    Scheduler scheduler{2};
    scheduler.pause();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<CountingItem>(&count));
    const uint64_t id = scheduler.submit(makeAction(std::move(items), 1));
    scheduler.cancelAction(id);
    scheduler.resume();

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    if (count.load(std::memory_order_acquire) != 0)
    {
        std::cerr << "cancel_paused: cancelled action still ran\n";
        return false;
    }

    scheduler.stop();
    return true;
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
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return true;
}

bool testIdempotentPauseResume()
{
    std::atomic<int> count{0};

    Scheduler scheduler{2};
    scheduler.pause();
    scheduler.pause();
    if (!waitPaused(scheduler, true, std::chrono::seconds(2)))
    {
        std::cerr << "idempotent: expected isPaused after pause\n";
        return false;
    }

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<CountingItem>(&count));
    scheduler.submit(makeAction(std::move(items), 1));

    scheduler.resume();
    scheduler.resume();
    if (!waitUntil(count, 1, std::chrono::seconds(2)))
    {
        std::cerr << "idempotent: action did not run\n";
        return false;
    }

    if (!waitPaused(scheduler, false, std::chrono::seconds(2)))
    {
        std::cerr << "idempotent: still paused after resume\n";
        return false;
    }

    scheduler.stop();
    return true;
}

bool testMultiChannelPause()
{
    std::atomic<int> a{0};
    std::atomic<int> b{0};

    Scheduler scheduler{4};
    scheduler.pause();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::vector<std::unique_ptr<ActionItem>> aItems;
    aItems.push_back(std::make_unique<CountingItem>(&a));
    std::vector<std::unique_ptr<ActionItem>> bItems;
    bItems.push_back(std::make_unique<CountingItem>(&b));
    scheduler.submit(makeAction(std::move(aItems), 1));
    scheduler.submit(makeAction(std::move(bItems), 2));

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    if (a.load() != 0 || b.load() != 0)
    {
        std::cerr << "multi_pause: channels advanced while paused\n";
        return false;
    }

    scheduler.resume();
    if (!waitUntil(a, 1, std::chrono::seconds(2)) || !waitUntil(b, 1, std::chrono::seconds(2)))
    {
        std::cerr << "multi_pause: channels did not run after resume\n";
        return false;
    }

    scheduler.stop();
    return true;
}

} // namespace

int main()
{
    using TestFn = bool (*)();
    const std::pair<const char *, TestFn> tests[] = {
        {"pause_stops_next_item", testPauseStopsNextItem},
        {"submit_while_paused", testSubmitWhilePaused},
        {"pause_freezes_delay", testPauseFreezesDelay},
        {"cancel_flush_while_paused", testCancelFlushWhilePaused},
        {"cancel_queued_while_paused", testCancelQueuedWhilePaused},
        {"idempotent_pause_resume", testIdempotentPauseResume},
        {"multi_channel_pause", testMultiChannelPause},
    };

    int failed = 0;
    for (const auto &[name, fn] : tests)
    {
        std::cout << "[RUN ] " << name << '\n';
        if (!fn())
        {
            std::cout << "[FAIL] " << name << '\n';
            ++failed;
        }
        else
        {
            std::cout << "[PASS] " << name << '\n';
        }
    }

    if (failed != 0)
    {
        std::cerr << failed << " phase8 pause smoke test(s) failed\n";
        return 1;
    }

    std::cout << "Phase 8 pause/resume smoke tests passed\n";
    return 0;
}
