/**
 * @file WorkerPool.hpp
 * @brief Executes ActionExecutionContexts on a pool of worker threads.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

#include "App/ActionExecutionContext.hpp"
#include "App/ThreadSafeQueue.hpp"

namespace Application
{

/**
 * @brief Outcome of a worker's run over an ActionExecutionContext.
 *
 * The mock/real scheduler consumes these. Nested scheduleAction requests stay
 * on the context for the scheduler to drain.
 */
struct WorkerResult
{
    enum class Status
    {
        Completed,
        Delayed,
        Cancelled
    };

    std::unique_ptr<ActionExecutionContext> context;
    Status status = Status::Completed;
    std::chrono::steady_clock::time_point wakeTime{};
};

/**
 * @brief Worker thread pool with no scheduling policy.
 *
 * Workers run ActionItems until the Action finishes, is cancelled, or an item
 * returns DelayUntil. They never sleep for delays.
 */
class WorkerPool
{
public:
    explicit WorkerPool(std::size_t workerCount);
    ~WorkerPool();

    WorkerPool(const WorkerPool &) = delete;
    WorkerPool &operator=(const WorkerPool &) = delete;

    void submit(std::unique_ptr<ActionExecutionContext> context);

    /// @brief Blocks until a worker posts a result, or the pool has stopped and drained.
    [[nodiscard]] bool waitResult(WorkerResult &out);

    void stop();

private:
    void workerMain();

    ThreadSafeQueue<std::unique_ptr<ActionExecutionContext>> m_inbound;
    ThreadSafeQueue<WorkerResult> m_outbound;
    std::vector<std::jthread> m_workers;
};

} // namespace Application
