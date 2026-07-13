/**
 * @file phase3_worker_smoke.cpp
 * @brief Disposable smoke test for WorkerPool (Phase 3).
 */

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/ActionItems.hpp"
#include "App/ExecuteResult.hpp"
#include "App/WorkerPool.hpp"

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
            m_counter->fetch_add(1, std::memory_order_relaxed);
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

std::unique_ptr<ActionExecutionContext> makeContext(std::vector<std::unique_ptr<ActionItem>> items)
{
    auto action = std::make_unique<Action>(std::move(items), "smoke", "", "smoke", 0);
    return std::make_unique<ActionExecutionContext>(std::move(action));
}

bool testCompleted()
{
    std::atomic<int> counter{0};
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<CountingItem>(&counter));
    items.push_back(std::make_unique<CountingItem>(&counter));
    items.push_back(std::make_unique<CountingItem>(&counter));

    WorkerPool pool{2};
    pool.submit(makeContext(std::move(items)));

    WorkerResult result;
    if (!pool.waitResult(result))
    {
        std::cerr << "completed: waitResult failed\n";
        return false;
    }

    pool.stop();

    if (result.status != WorkerResult::Status::Completed)
    {
        std::cerr << "completed: expected Completed\n";
        return false;
    }
    if (counter.load() != 3)
    {
        std::cerr << "completed: expected 3 executions, got " << counter.load() << '\n';
        return false;
    }
    if (result.context == nullptr || !result.context->finished())
    {
        std::cerr << "completed: context should be finished\n";
        return false;
    }
    return true;
}

bool testDelayedDoesNotSleep()
{
    constexpr auto delay = std::chrono::seconds(5);

    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<DelayItem>(delay));
    items.push_back(std::make_unique<CountingItem>(nullptr));

    WorkerPool pool{1};

    const auto start = std::chrono::steady_clock::now();
    pool.submit(makeContext(std::move(items)));

    WorkerResult result;
    if (!pool.waitResult(result))
    {
        std::cerr << "delayed: waitResult failed\n";
        return false;
    }
    const auto elapsed = std::chrono::steady_clock::now() - start;

    if (result.status != WorkerResult::Status::Delayed)
    {
        std::cerr << "delayed: expected Delayed\n";
        return false;
    }
    if (elapsed > std::chrono::milliseconds(500))
    {
        std::cerr << "delayed: worker slept too long ("
                  << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
                  << " ms)\n";
        return false;
    }
    if (result.context == nullptr || result.context->finished())
    {
        std::cerr << "delayed: context should still have remaining items\n";
        return false;
    }

    // Resume as the scheduler would after the delay elapses.
    pool.submit(std::move(result.context));

    WorkerResult resumed;
    if (!pool.waitResult(resumed) || resumed.status != WorkerResult::Status::Completed)
    {
        std::cerr << "delayed: resume did not complete\n";
        return false;
    }

    pool.stop();
    return true;
}

bool testParallelCompleted()
{
    constexpr int jobCount = 8;
    std::atomic<int> counter{0};

    WorkerPool pool{4};
    for (int i = 0; i < jobCount; ++i)
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<CountingItem>(&counter));
        pool.submit(makeContext(std::move(items)));
    }

    int completed = 0;
    for (int i = 0; i < jobCount; ++i)
    {
        WorkerResult result;
        if (!pool.waitResult(result) || result.status != WorkerResult::Status::Completed)
        {
            std::cerr << "parallel: missing Completed result\n";
            return false;
        }
        ++completed;
    }

    pool.stop();

    if (completed != jobCount || counter.load() != jobCount)
    {
        std::cerr << "parallel: expected " << jobCount << " jobs, got completed=" << completed
                  << " counter=" << counter.load() << '\n';
        return false;
    }
    return true;
}

bool testCancelled()
{
    std::atomic<int> counter{0};
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<CountingItem>(&counter));
    items.push_back(std::make_unique<CountingItem>(&counter));

    auto context = makeContext(std::move(items));
    context->cancel();

    WorkerPool pool{1};
    pool.submit(std::move(context));

    WorkerResult result;
    if (!pool.waitResult(result))
    {
        std::cerr << "cancelled: waitResult failed\n";
        return false;
    }
    pool.stop();

    if (result.status != WorkerResult::Status::Cancelled)
    {
        std::cerr << "cancelled: expected Cancelled\n";
        return false;
    }
    if (counter.load() != 0)
    {
        std::cerr << "cancelled: items should not have run\n";
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
        {"completed", testCompleted},
        {"delayed_no_sleep", testDelayedDoesNotSleep},
        {"parallel", testParallelCompleted},
        {"cancelled", testCancelled},
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

    std::cout << "Phase 3 WorkerPool smoke tests passed\n";
    return 0;
}
