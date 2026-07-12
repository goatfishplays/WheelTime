/**
 * @file Scheduler.hpp
 * @brief Scheduler thread: owns contexts, channels, delays, cancel, and worker dispatch.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

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
 * Channel rules:
 * - At most one Action in-flight per channel key.
 * - Delayed Actions **hold** their channel until wake (strict sequentialness).
 * - Waiting Actions preserve earliestStart (nested future schedules stay correct).
 * - Channel 0 assigns a private key per Action so those Actions never block
 *   one another.
 *
 * Delay rules:
 * - Workers never sleep; DelayUntil returns the context to the scheduler.
 * - The scheduler thread waits on its mailbox until a new event, the earliest
 *   DelayQueue wake time, or stop.
 *
 * Cancellation:
 * - cancelAction / cancelChannel / cancelAll (thread-safe).
 * - Only between ActionItems; the current item always finishes.
 * - Actions with cancelable()==false are skipped.
 * - setCancelFlush Actions run immediately on cancel (e.g. KeyUp).
 * - scheduleAction(..., removeIfParentCancelled=true) links delayed cleanup
 *   that is discarded when the parent is cancelled (after flush).
 *
 * Pause/resume is not implemented yet (later phase).
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
     *
     * @return Runtime action id used with cancelAction().
     */
    uint64_t submit(std::unique_ptr<Action> action);

    /**
     * @brief Copies @p action into a new owned Action and submits it.
     *
     * Intended for library Actions that must remain in the caller's store.
     *
     * @return Runtime action id used with cancelAction().
     */
    uint64_t submit(const Action &action);

    /**
     * @brief Cancels one Action by runtime id.
     *
     * No-op if unknown or uncancelable. Queued work is dropped; delayed work is
     * discarded (channel freed); executing work finishes the current item then stops.
     */
    void cancelAction(uint64_t actionId);

    /**
     * @brief Cancels every cancelable Action whose Action::channel() equals @p channel.
     *
     * Channel 0 cancels all logical channel-0 Actions (each has a private key).
     * Uncancelable Actions on that channel are left alone.
     */
    void cancelChannel(uint32_t channel);

    /**
     * @brief Cancels every cancelable Action and runs registered cancel-flush cleanups.
     *
     * Uncancelable Actions (e.g. delayed KeyUp safety nets) are not cancelled.
     */
    void cancelAll();

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
        /// @brief Preassigned runtime id (from submit() or nested schedule ingest).
        uint64_t actionId = 0;
        /// @brief Parent that scheduled this (0 if top-level submit).
        uint64_t parentActionId = 0;
        /// @brief If true, record this id under the parent for cancel-linked discard.
        bool removeIfParentCancelled = false;
    };

    /// @brief Mailbox payload for cancelAction().
    struct CancelActionRequest
    {
        uint64_t actionId = 0;
    };

    /// @brief Mailbox payload for cancelChannel().
    struct CancelChannelRequest
    {
        uint32_t channel = 0;
    };

    /// @brief Mailbox payload for cancelAll().
    struct CancelAllRequest
    {
    };

    /// @brief Unified mailbox: submits, worker outcomes, and cancel requests.
    using SchedulerEvent = std::variant<
        SubmitRequest,
        WorkerResult,
        CancelActionRequest,
        CancelChannelRequest,
        CancelAllRequest>;

    /// @brief Scheduler-thread loop: due delays, mailbox wait, event handling.
    void threadMain();

    /// @brief Admits a submitted Action onto its channel (or parks until earliestStart).
    void handleSubmit(SubmitRequest request);

    /**
     * @brief Applies a worker outcome: nested schedules, delay park, resume, or channel release.
     */
    void handleResult(WorkerResult result);

    /// @brief Implements cancelAction on the scheduler thread.
    void handleCancelAction(uint64_t actionId);

    /// @brief Implements cancelChannel on the scheduler thread.
    void handleCancelChannel(uint32_t channel);

    /// @brief Implements cancelAll on the scheduler thread.
    void handleCancelAll();

    /// @brief Applies a ChannelDispatch (run now vs park on DelayQueue).
    void applyDispatch(ChannelDispatch dispatch);

    /// @brief Dispatches any DelayQueue contexts whose wake time has been reached.
    void processDueDelays();

    /**
     * @brief Turns context.scheduleAction(...) requests into SubmitRequests.
     */
    void ingestScheduledActions(ActionExecutionContext &context);

    /**
     * @brief Moves context.setCancelFlush(...) Actions into the cleanup registry.
     */
    void ingestCancelFlushes(ActionExecutionContext &context);

    /// @brief Tracks cancel flag / cancelable / logical channel for a live context.
    void registerContext(const ActionExecutionContext &context);

    /// @brief Drops tracking tables for a finished or discarded Action.
    void unregisterContext(uint64_t actionId);

    /// @brief Submits registered cancel-flush Actions immediately (forced uncancelable).
    void flushCleanups(uint64_t actionId);

    /**
     * @brief Unbinds and forgets @p context; optionally frees its channel and dispatches next.
     */
    void discardContext(std::unique_ptr<ActionExecutionContext> context, bool releaseChannel);

    /// @brief Looks up whether @p actionId may be cancelled (default true if unknown).
    [[nodiscard]] bool isCancelableAction(uint64_t actionId) const;

    /// @brief Allocates a new runtime action id for submit / nested schedules.
    [[nodiscard]] uint64_t allocateActionId();

    ThreadSafeQueue<SchedulerEvent> m_mailbox;
    WorkerPool m_workers;
    std::jthread m_thread;
    std::atomic<uint64_t> m_nextActionId{1};

    // Touched only by the scheduler thread:
    ChannelManager m_channels;
    DelayQueue m_delayQueue;
    /// @brief Shared cancel flags for in-flight contexts (worker-visible).
    std::unordered_map<uint64_t, std::shared_ptr<std::atomic<bool>>> m_cancelFlags;
    /// @brief Whether each known Action may be cancelled.
    std::unordered_map<uint64_t, bool> m_cancelableById;
    /// @brief Action::channel() for cancelChannel matching (including channel 0).
    std::unordered_map<uint64_t, uint32_t> m_logicalChannelById;
    /// @brief Immediate cleanup Actions keyed by parent action id.
    std::unordered_map<uint64_t, std::vector<std::unique_ptr<Action>>> m_flushActions;
    /// @brief Delayed cleanup action ids to discard when a parent is cancelled.
    std::unordered_map<uint64_t, std::vector<uint64_t>> m_linkedDelayedByParent;
};

} // namespace Application
