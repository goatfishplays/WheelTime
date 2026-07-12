/**
 * @file phase5_delay_queue_smoke.cpp
 * @brief Disposable smoke test for DelayQueue (Phase 5).
 */

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/ActionItems.hpp"
#include "App/DelayQueue.hpp"
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

std::unique_ptr<ActionExecutionContext> makeEmptyContext()
{
    auto action = std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "delay", "", "delay", 0);
    return std::make_unique<ActionExecutionContext>(std::move(action));
}

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

bool testOrderingAndPopDue()
{
    DelayQueue queue;
    const auto t0 = std::chrono::steady_clock::now();

    auto early = makeEmptyContext();
    auto mid = makeEmptyContext();
    auto late = makeEmptyContext();
    const uint64_t earlyId = early->actionId();
    const uint64_t midId = mid->actionId();
    const uint64_t lateId = late->actionId();

    queue.push(t0 + std::chrono::milliseconds(300), std::move(late));
    queue.push(t0 + std::chrono::milliseconds(100), std::move(early));
    queue.push(t0 + std::chrono::milliseconds(200), std::move(mid));

    if (queue.size() != 3 || !queue.nextWakeTime().has_value()
        || *queue.nextWakeTime() != t0 + std::chrono::milliseconds(100))
    {
        std::cerr << "ordering: nextWakeTime incorrect\n";
        return false;
    }

    auto none = queue.popDue(t0);
    if (!none.empty())
    {
        std::cerr << "ordering: popDue too early\n";
        return false;
    }

    auto first = queue.popDue(t0 + std::chrono::milliseconds(100));
    if (first.size() != 1 || first[0]->actionId() != earlyId)
    {
        std::cerr << "ordering: expected early context first\n";
        return false;
    }

    auto rest = queue.popDue(t0 + std::chrono::milliseconds(500));
    if (rest.size() != 2 || rest[0]->actionId() != midId || rest[1]->actionId() != lateId)
    {
        std::cerr << "ordering: expected mid then late\n";
        return false;
    }

    if (!queue.empty())
    {
        std::cerr << "ordering: queue should be empty\n";
        return false;
    }
    return true;
}

bool testEqualWakeFifo()
{
    DelayQueue queue;
    const auto wake = std::chrono::steady_clock::now() + std::chrono::milliseconds(50);

    auto a = makeEmptyContext();
    auto b = makeEmptyContext();
    const uint64_t aId = a->actionId();
    const uint64_t bId = b->actionId();

    queue.push(wake, std::move(a));
    queue.push(wake, std::move(b));

    auto due = queue.popDue(wake);
    if (due.size() != 2 || due[0]->actionId() != aId || due[1]->actionId() != bId)
    {
        std::cerr << "fifo: equal wake times should preserve push order\n";
        return false;
    }
    return true;
}

bool testSchedulerMultiDelay()
{
    std::atomic<int> counter{0};
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(80)));
    items.push_back(std::make_unique<DelayItem>(std::chrono::milliseconds(80)));
    items.push_back(std::make_unique<CountingItem>(&counter));

    Scheduler scheduler{2};
    const auto start = std::chrono::steady_clock::now();
    scheduler.submit(std::make_unique<Action>(std::move(items), "multi", "", "multi", 1));

    if (!waitUntil(counter, 1, std::chrono::seconds(3)))
    {
        std::cerr << "multi-delay: timed out\n";
        return false;
    }

    const auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed < std::chrono::milliseconds(140))
    {
        std::cerr << "multi-delay: finished too early\n";
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
        {"ordering_pop_due", testOrderingAndPopDue},
        {"equal_wake_fifo", testEqualWakeFifo},
        {"scheduler_multi_delay", testSchedulerMultiDelay},
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

    std::cout << "Phase 5 DelayQueue smoke tests passed\n";
    return 0;
}
