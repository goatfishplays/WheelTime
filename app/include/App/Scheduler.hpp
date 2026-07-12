/**
 * @file Scheduler.hpp
 * @brief Scheduler thread: owns contexts, channels, delays, and worker dispatch.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <utility>
#include <variant>

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/ChannelManager.hpp"
#include "App/DelayQueue.hpp"
#include "App/ThreadSafeQueue.hpp"
#include "App/WorkerPool.hpp"

namespace Application
{

/**
 * @brief Kinds of requests ActionItems may record for the scheduler (future use).
 */
enum class SchedulerRequestType
{
    ScheduleAction,
    CancelAction,
    CancelChannel
};

/**
 * @brief Accepts Actions and runs them via WorkerPool with channel + delay policy.
 *
 * The scheduler is the only component that makes scheduling decisions. Callers
 * submit work from any thread; a dedicated scheduler thread owns every
 * ActionExecutionContext, enforces channel FIFO via ChannelManager, parks
 * Delayed contexts on DelayQueue, and dispatches runnable work to WorkerPool.
 *
 * Channel rules (Phase 6):
 * - At most one Action in-flight per channel key.
 * - Delayed Actions **hold** their channel until wake (strict sequentialness).
 * - Waiting Actions preserve earliestStart (nested future schedules stay correct).
 * - Channel 0 assigns a private key per Action so those Actions never block
 *   one another.
 *
 * Delay rules (Phase 5):
 * - Workers never sleep; DelayUntil returns the context to the scheduler.
 * - The scheduler thread waits on its mailbox until a new event, the earliest
 *   DelayQueue wake time, or stop.
 *
 * Cancellation and pause are not implemented yet (later phases).
 */
class Scheduler
{
public:
    /**
     * @brief Starts the scheduler thread and an owned WorkerPool.
     * @param workerCount Number of worker threads used for ActionItem execution.
     */
    explicit Scheduler(std::size_t workerCount = 2);

    /**
     * @brief Stops workers and the scheduler thread, then joins.
     */
    ~Scheduler();

    Scheduler(const Scheduler &) = delete;
    Scheduler &operator=(const Scheduler &) = delete;

    /**
     * @brief Enqueues @p action for scheduling (thread-safe).
     *
     * Takes ownership. The Action is wrapped in an ActionExecutionContext on
     * the scheduler thread.
     */
    void submit(std::unique_ptr<Action> action);

    /**
     * @brief Copies @p action into a new owned Action and submits it.
     *
     * Intended for library Actions that must remain in the caller's store.
     */
    void submit(const Action &action);

    /**
     * @brief Stops accepting new worker results, drains the mailbox, and joins.
     *
     * Safe to call multiple times. Invoked automatically from the destructor.
     */
    void stop();

private:
    /**
     * @brief Mailbox payload for a newly submitted Action.
     */
    struct SubmitRequest
    {
        std::unique_ptr<Action> action;
        /// @brief If in the future, the Action is admitted but not dispatched until this time.
        std::chrono::steady_clock::time_point earliestStart{};
    };

    /// @brief Unified mailbox: external submits and worker outcomes.
    using SchedulerEvent = std::variant<SubmitRequest, WorkerResult>;

    /// @brief Scheduler-thread loop: due delays, mailbox wait, event handling.
    void threadMain();

    /// @brief Admits a submitted Action onto its channel (or parks until earliestStart).
    void handleSubmit(SubmitRequest request);

    /**
     * @brief Applies a worker outcome: nested schedules, delay park, or channel release.
     */
    void handleResult(WorkerResult result);

    /// @brief Applies a ChannelDispatch (run now vs park on DelayQueue).
    void applyDispatch(ChannelDispatch dispatch);

    /// @brief Dispatches any DelayQueue contexts whose wake time has been reached.
    void processDueDelays();

    /**
     * @brief Turns context.scheduleAction(...) requests into SubmitRequests.
     */
    void ingestScheduledActions(ActionExecutionContext &context);

    ThreadSafeQueue<SchedulerEvent> m_mailbox;
    WorkerPool m_workers;
    std::jthread m_thread;

    // Touched only by the scheduler thread:
    ChannelManager m_channels;
    DelayQueue m_delayQueue;
};

} // namespace Application
