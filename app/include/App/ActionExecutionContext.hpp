/**
 * @file ActionExecutionContext.hpp
 * @brief Runtime execution state for a single Action.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "App/Action.hpp"

namespace Application
{

class ActionItem;

/**
 * @brief An Action recorded during execute() for the scheduler to submit later.
 *
 * ActionItems call ActionExecutionContext::scheduleAction instead of talking to
 * the Scheduler directly. The scheduler drains these after execute() returns
 * (when the worker yields Continue / Delayed / Completed).
 */
struct ScheduledAction
{
    std::unique_ptr<Action> action;
    std::chrono::steady_clock::time_point wakeTime;
    /// @brief Parent that scheduled this (for cancel-linked cleanup).
    uint64_t parentActionId = 0;
    /// @brief If true, discard this from DelayQueue/waiting when the parent is cancelled.
    bool removeIfParentCancelled = false;
};

/**
 * @brief Cancel request recorded by ActionItems for the scheduler to apply.
 *
 * Mirrors Scheduler cancel APIs. ActionItems must not call the Scheduler
 * directly; they enqueue these and yield.
 */
struct PendingCancelRequest
{
    enum class Level
    {
        /// @brief Cancel the most recently submitted Action (see channel / excludeActionId).
        MostRecent,
        Channel,
        All
    };

    Level level = Level::MostRecent;
    /// @brief Runtime id to skip when level == MostRecent (usually the requester).
    uint64_t excludeActionId = 0;
    /// @brief Channel filter: for MostRecent, 0 = any channel; for Channel, the target.
    uint32_t channel = 0;
};

/**
 * @brief Owns an Action and tracks progress while workers run its items.
 *
 * Workers execute contexts, not bare Actions. This type holds:
 * - the Action (ownership),
 * - the current ActionItem index,
 * - shared cancel state (so the scheduler can cancel an in-flight Action),
 * - pending ScheduledAction requests,
 * - pending cancel requests (cancelAction / cancelChannel / cancelAll),
 * - cancel-flush Actions (run immediately when this Action is cancelled).
 */
class ActionExecutionContext
{
public:
    /**
     * @brief Takes ownership of @p action.
     * @param actionId Preassigned runtime id; if 0, a new id is allocated.
     * @throws std::invalid_argument if @p action is null.
     */
    explicit ActionExecutionContext(std::unique_ptr<Action> action, uint64_t actionId = 0);

    ActionExecutionContext(const ActionExecutionContext &) = delete;
    ActionExecutionContext &operator=(const ActionExecutionContext &) = delete;

    ActionExecutionContext(ActionExecutionContext &&) noexcept = default;
    ActionExecutionContext &operator=(ActionExecutionContext &&) noexcept = default;

    ~ActionExecutionContext() = default;

    /// @brief Runtime instance id (distinct from Action's config/library string id).
    [[nodiscard]] uint64_t actionId() const noexcept;

    /// @brief Forwards Action::channel() for the owned Action.
    [[nodiscard]] uint32_t channel() const noexcept;

    /// @brief Forwards Action::cancelable().
    [[nodiscard]] bool isCancelable() const noexcept;

    /// @brief Whether cancel() was requested; checked between ActionItems.
    [[nodiscard]] bool isCancelled() const noexcept;

    /// @brief Marks this execution cancelled. Takes effect between ActionItems.
    void cancel() noexcept;

    /// @brief Shared flag so the scheduler can cancel while a worker owns this context.
    [[nodiscard]] std::shared_ptr<std::atomic<bool>> cancelFlag() const noexcept;

    [[nodiscard]] Action &action() noexcept;
    [[nodiscard]] const Action &action() const noexcept;

    /**
     * @brief Item at the current index, or nullptr if finished().
     */
    [[nodiscard]] ActionItem *currentItem() noexcept;
    [[nodiscard]] const ActionItem *currentItem() const noexcept;

    /// @brief Zero-based index of the next/current ActionItem.
    [[nodiscard]] size_t currentIndex() const noexcept;

    /// @brief Advances to the next item without executing it.
    void advance() noexcept;

    /// @brief True when the current index is past the last item.
    [[nodiscard]] bool finished() const noexcept;

    /**
     * @brief Records an Action for the scheduler to pick up after execute() returns.
     * @param action Nested work to submit (ignored if null).
     * @param wakeTime Earliest time the scheduler should start it.
     * @param removeIfParentCancelled If true, cancel of this context removes that child.
     */
    void scheduleAction(std::unique_ptr<Action> action,
                        std::chrono::steady_clock::time_point wakeTime,
                        bool removeIfParentCancelled = false);

    /**
     * @brief Registers cleanup to run immediately when this Action is cancelled.
     *
     * Typical use: KeyDown schedules a delayed KeyUp and also setCancelFlush(KeyUp)
     * so cancel releases the key without waiting for the hold delay.
     */
    void setCancelFlush(std::unique_ptr<Action> action);

    /// @brief Requests cancel of the most recent Action on @p channel (0 = any).
    ///
    /// The requesting Action (this context) is excluded so Cancel Latest never
    /// cancels itself.
    void requestCancelMostRecent(uint32_t channel);

    /// @brief Requests cancelChannel(@p channel) after execute() returns.
    void requestCancelChannel(uint32_t channel);

    /// @brief Requests cancelAll() after execute() returns.
    void requestCancelAll();

    /// @brief Pending nested submits (scheduler should prefer takeScheduledActions()).
    [[nodiscard]] const std::vector<ScheduledAction> &scheduledActions() const noexcept;

    /// @brief Moves out all pending schedule requests and clears the list.
    [[nodiscard]] std::vector<ScheduledAction> takeScheduledActions() noexcept;

    /// @brief Drops pending nested submits without returning them.
    void clearScheduledActions() noexcept;

    /// @brief Moves out cancel-flush Actions and clears the list.
    [[nodiscard]] std::vector<std::unique_ptr<Action>> takeCancelFlushes() noexcept;

    /// @brief Moves out pending cancel requests and clears the list.
    [[nodiscard]] std::vector<PendingCancelRequest> takeCancelRequests() noexcept;

    /// @brief True if scheduleAction / setCancelFlush / cancel requests are pending.
    [[nodiscard]] bool hasPendingSchedulerRequests() const noexcept;

    /**
     * @brief Marks this context as a cancel-flush cleanup Action.
     *
     * Set by the scheduler when submitting setCancelFlush work so workers can
     * log flush execution separately from normal runs.
     */
    void markCancelFlush() noexcept;

    /// @brief True if this Action was submitted as cancel-flush cleanup.
    [[nodiscard]] bool isCancelFlush() const noexcept;

private:
    std::unique_ptr<Action> m_action;
    uint64_t m_actionId = 0;
    size_t m_currentIndex = 0;
    std::shared_ptr<std::atomic<bool>> m_cancelFlag;
    std::vector<ScheduledAction> m_scheduledActions;
    std::vector<std::unique_ptr<Action>> m_cancelFlushes;
    std::vector<PendingCancelRequest> m_cancelRequests;
    bool m_isCancelFlush = false;
};

} // namespace Application
