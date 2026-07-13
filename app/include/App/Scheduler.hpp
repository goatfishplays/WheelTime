/**
 * @file Scheduler.hpp
 * @brief Scheduler thread: owns contexts, channels, delays, cancel, pause, and worker dispatch.
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
 * @brief Kinds of requests ActionItems may record for the scheduler.
 */
enum class SchedulerRequestType
{
    ScheduleAction,
    CancelMostRecent,
    CancelChannel,
    CancelAll
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
 * - While paused, due delays are not dispatched; on resume, pending wake times
 *   are shifted forward by the pause duration so remaining delay is preserved.
 *
 * Cancellation:
 * - cancelAction / cancelChannel / cancelAll (thread-safe).
 * - Only between ActionItems; the current item always finishes.
 * - Actions with cancelable()==false are skipped.
 * - setCancelFlush Actions run immediately on cancel (e.g. KeyUp).
 * - scheduleAction(..., removeIfParentCancelled=true) links delayed cleanup
 *   that is discarded when the parent is cancelled (after flush).
 * - Cancel-flush bypasses pause so hardware cleanup can still run.
 *
 * Pause/resume:
 * - pause() stops dispatch of new work; submits are still admitted.
 * - Delayed Actions stop progressing (virtual clock).
 * - Workers finish the current ActionItem then sleep until resume.
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
     * the scheduler thread. While paused, the Action is admitted but will not
     * start until resume().
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
     * Works while paused. Cancel-flush cleanups still run immediately.
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
     * @brief Stops dispatching new work; freezes delay progress (thread-safe).
     *
     * In-flight ActionItems finish; workers then sleep. Submits are still
     * admitted but do not start until resume(). Idempotent.
     */
    void pause();

    /**
     * @brief Resumes dispatch and delay progress after pause() (thread-safe).
     *
     * Idempotent if not paused.
     */
    void resume();

    /// @brief Whether pause() is in effect (approximate; safe from any thread).
    [[nodiscard]] bool isPaused() const noexcept;

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
        /// @brief If true, RunNow bypasses pause (cancel-flush cleanups).
        bool forceDispatch = false;
        /// @brief If true, this submit is cancel-flush cleanup (for logging / marking).
        bool isCancelFlush = false;
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

    /// @brief Mailbox payload for pause().
    struct PauseRequest
    {
    };

    /// @brief Mailbox payload for resume().
    struct ResumeRequest
    {
    };

    /// @brief Unified mailbox: submits, worker outcomes, cancel, and pause.
    using SchedulerEvent = std::variant<
        SubmitRequest,
        WorkerResult,
        CancelActionRequest,
        CancelChannelRequest,
        CancelAllRequest,
        PauseRequest,
        ResumeRequest>;

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

    /**
     * @brief Cancels the most recently submitted cancelable Action.
     * @param channel If non-zero, only consider Actions on that logical channel.
     * @param excludeActionId Never cancel this id (the requesting Action).
     */
    void handleCancelMostRecent(uint32_t channel, uint64_t excludeActionId);

    /// @brief Implements pause on the scheduler thread.
    void handlePause();

    /// @brief Implements resume on the scheduler thread.
    void handleResume();

    /**
     * @brief Applies a ChannelDispatch (run now vs park on DelayQueue).
     * @param forceDispatch If true, RunNow ignores pause (cancel-flush).
     */
    void applyDispatch(ChannelDispatch dispatch, bool forceDispatch = false);

    /**
     * @brief Hands @p context to WorkerPool, or holds it while paused.
     * @param force If true, submit even while the scheduler is paused.
     */
    void dispatchToWorker(std::unique_ptr<ActionExecutionContext> context, bool force = false);

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

    /**
     * @brief Applies context.requestCancel*(...) via handleCancel*.
     */
    void ingestCancelRequests(ActionExecutionContext &context);

    /// @brief Tracks cancel flag / cancelable / logical channel for a live context.
    void registerContext(const ActionExecutionContext &context);

    /// @brief Drops tracking tables for a finished or discarded Action.
    void unregisterContext(uint64_t actionId);

    /// @brief Records @p actionId as the newest live submission (scheduler thread).
    void noteSubmitOrder(uint64_t actionId);

    /// @brief Removes @p actionId from the live submission order.
    void forgetSubmitOrder(uint64_t actionId);

    /// @brief Submits registered cancel-flush Actions immediately (forced uncancelable).
    void flushCleanups(uint64_t actionId);

    /**
     * @brief Unbinds and forgets @p context; optionally frees its channel and dispatches next.
     */
    void discardContext(std::unique_ptr<ActionExecutionContext> context, bool releaseChannel);

    /// @brief Removes a held ready context if cancel targets it while paused.
    void discardPausedReady(uint64_t actionId);

    /// @brief Looks up whether @p actionId may be cancelled (default true if unknown).
    [[nodiscard]] bool isCancelableAction(uint64_t actionId) const;

    /// @brief Allocates a new runtime action id for submit / nested schedules.
    [[nodiscard]] uint64_t allocateActionId();

    ThreadSafeQueue<SchedulerEvent> m_mailbox;
    WorkerPool m_workers;
    std::jthread m_thread;
    std::atomic<uint64_t> m_nextActionId{1};
    std::atomic<bool> m_pausedFlag{false};

    // Touched only by the scheduler thread:
    ChannelManager m_channels;
    DelayQueue m_delayQueue;
    bool m_paused = false;
    std::chrono::steady_clock::time_point m_pausedAt{};
    /// @brief RunNow contexts held while paused; drained on resume.
    std::vector<std::unique_ptr<ActionExecutionContext>> m_pausedReady;
    /// @brief Shared cancel flags for in-flight contexts (worker-visible).
    std::unordered_map<uint64_t, std::shared_ptr<std::atomic<bool>>> m_cancelFlags;
    /// @brief Whether each known Action may be cancelled.
    std::unordered_map<uint64_t, bool> m_cancelableById;
    /// @brief Action::channel() for cancelChannel matching (including channel 0).
    std::unordered_map<uint64_t, uint32_t> m_logicalChannelById;
    /// @brief Live Action ids in submit order (oldest → newest).
    std::vector<uint64_t> m_submitOrder;
    /// @brief Immediate cleanup Actions keyed by parent action id.
    std::unordered_map<uint64_t, std::vector<std::unique_ptr<Action>>> m_flushActions;
    /// @brief Delayed cleanup action ids to discard when a parent is cancelled.
    std::unordered_map<uint64_t, std::vector<uint64_t>> m_linkedDelayedByParent;
};

} // namespace Application
