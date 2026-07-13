/**
 * @file WorkerPool.hpp
 * @brief Executes ActionExecutionContexts on a pool of worker threads.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "App/ActionExecutionContext.hpp"
#include "App/ThreadSafeQueue.hpp"

namespace Application
{

/**
 * @brief Outcome of a worker's run over an ActionExecutionContext.
 *
 * Produced when the worker needs the scheduler: the Action finished, was
 * cancelled between items, an item returned DelayUntil, or an item recorded
 * scheduleAction / setCancelFlush side effects (Continue). Nested requests
 * remain on @ref context for the scheduler to drain — workers never process them.
 */
struct WorkerResult
{
    enum class Status
    {
        Completed, ///< All items ran (or none remained).
        Continue,  ///< Finished one item with pending scheduler requests; resume after ingest.
        Delayed,   ///< Stopped after DelayUntil; @ref wakeTime is set; channel should be held.
        Cancelled  ///< Stopped because the context was cancelled between ActionItems.
    };

    std::unique_ptr<ActionExecutionContext> context;
    Status status = Status::Completed;
    /// @brief Valid when @ref status is Delayed.
    std::chrono::steady_clock::time_point wakeTime{};
};

/**
 * @brief Worker thread pool with no scheduling policy.
 *
 * Workers:
 * - Wait for ActionExecutionContexts on an inbound queue.
 * - Execute ActionItems until finish, cancel, DelayUntil, or pending
 *   scheduleAction / setCancelFlush side effects (then yield Continue).
 * - Never sleep for delays and never decide channels / ordering.
 * - Return a WorkerResult via @ref waitResult or an optional result handler.
 * - When paused, finish the current ActionItem then do not take new inbound work.
 * - Cancel-flush cleanups may still run via submitUrgent().
 */
class WorkerPool
{
public:
    /**
     * @brief Creates @p workerCount workers; results are retrieved with waitResult().
     */
    explicit WorkerPool(std::size_t workerCount);

    /**
     * @brief Creates @p workerCount workers and delivers each result to @p resultHandler.
     *
     * Used by Scheduler to feed a unified mailbox. When a handler is set,
     * results are not stored for waitResult().
     */
    WorkerPool(std::size_t workerCount, std::function<void(WorkerResult)> resultHandler);

    /// @brief Stops the pool and joins all workers.
    ~WorkerPool();

    WorkerPool(const WorkerPool &) = delete;
    WorkerPool &operator=(const WorkerPool &) = delete;

    /**
     * @brief Queues @p context for a worker (thread-safe).
     * Ignored if the pool has been stopped or @p context is null.
     */
    void submit(std::unique_ptr<ActionExecutionContext> context);

    /**
     * @brief Queues @p context that may run even while the pool is paused.
     *
     * Used for cancel-flush cleanups so KeyUp-style work can run during pause.
     */
    void submitUrgent(std::unique_ptr<ActionExecutionContext> context);

    /**
     * @brief When paused, workers finish the current item then stop taking inbound work.
     *
     * Thread-safe. Idempotent. Used by Scheduler pause/resume; cancel-flush may
     * still run via submitUrgent().
     */
    void setPaused(bool paused);

    /// @brief Whether setPaused(true) is in effect.
    [[nodiscard]] bool isPaused() const noexcept;

    /**
     * @brief Blocks until a worker posts a result, or the pool has stopped and drained.
     * @return false if stopped and no results remain (only when no result handler is set).
     */
    [[nodiscard]] bool waitResult(WorkerResult &out);

    /**
     * @brief Stops the inbound queue, joins workers, then stops the outbound queue.
     */
    void stop();

private:
    /// @brief Per-worker loop: pop context, run items, emit WorkerResult.
    void workerMain();

    /// @brief Delivers to the result handler if set, otherwise to @ref m_outbound.
    void deliverResult(WorkerResult result);

    ThreadSafeQueue<std::unique_ptr<ActionExecutionContext>> m_inbound;
    ThreadSafeQueue<std::unique_ptr<ActionExecutionContext>> m_urgent;
    ThreadSafeQueue<WorkerResult> m_outbound;
    std::function<void(WorkerResult)> m_resultHandler;
    std::vector<std::jthread> m_workers;

    mutable std::mutex m_pauseMutex;
    std::condition_variable m_pauseCv;
    std::atomic<bool> m_paused{false};
};

} // namespace Application
