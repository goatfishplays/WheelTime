/**
 * @file ActionExecutionContext.hpp
 * @brief Runtime execution state for a single Action.
 */

#pragma once

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
 * the Scheduler directly. The scheduler drains these after a worker returns.
 */
struct ScheduledAction
{
    std::unique_ptr<Action> action;
    std::chrono::steady_clock::time_point wakeTime;
};

/**
 * @brief Owns an Action and tracks progress while workers run its items.
 *
 * Workers execute contexts, not bare Actions. This type holds:
 * - the Action (ownership),
 * - the current ActionItem index,
 * - cancel state,
 * - pending ScheduledAction requests.
 *
 * It contains no scheduling policy (channels, delays, worker choice).
 */
class ActionExecutionContext
{
public:
    /**
     * @brief Takes ownership of @p action and assigns a runtime actionId.
     * @throws std::invalid_argument if @p action is null.
     */
    explicit ActionExecutionContext(std::unique_ptr<Action> action);

    ActionExecutionContext(const ActionExecutionContext &) = delete;
    ActionExecutionContext &operator=(const ActionExecutionContext &) = delete;

    ActionExecutionContext(ActionExecutionContext &&) noexcept = default;
    ActionExecutionContext &operator=(ActionExecutionContext &&) noexcept = default;

    ~ActionExecutionContext() = default;

    /// @brief Runtime instance id (distinct from Action's config/library string id).
    [[nodiscard]] uint64_t actionId() const noexcept;

    /// @brief Forwards Action::channel() for the owned Action.
    [[nodiscard]] uint32_t channel() const noexcept;

    /// @brief Whether cancel() was requested; checked between ActionItems.
    [[nodiscard]] bool isCancelled() const noexcept;

    /// @brief Marks this execution cancelled. Takes effect between ActionItems.
    void cancel() noexcept;

    [[nodiscard]] Action &action() noexcept;
    [[nodiscard]] const Action &action() const noexcept;

    /**
     * @brief Item at the current index, or nullptr if finished().
     */
    [[nodiscard]] ActionItem *currentItem() noexcept;
    [[nodiscard]] const ActionItem *currentItem() const noexcept;

    /// @brief Advances to the next item without executing it.
    void advance() noexcept;

    /// @brief True when the current index is past the last item.
    [[nodiscard]] bool finished() const noexcept;

    /**
     * @brief Records an Action for the scheduler to pick up after execute() returns.
     * @param action Nested work to submit (ignored if null).
     * @param wakeTime Earliest time the scheduler should start it.
     */
    void scheduleAction(std::unique_ptr<Action> action,
                        std::chrono::steady_clock::time_point wakeTime);

    /// @brief Pending nested submits (scheduler should prefer takeScheduledActions()).
    [[nodiscard]] const std::vector<ScheduledAction> &scheduledActions() const noexcept;

    /// @brief Moves out all pending schedule requests and clears the list.
    [[nodiscard]] std::vector<ScheduledAction> takeScheduledActions() noexcept;

    /// @brief Drops pending nested submits without returning them.
    void clearScheduledActions() noexcept;

private:
    std::unique_ptr<Action> m_action;
    uint64_t m_actionId = 0;
    size_t m_currentIndex = 0;
    bool m_cancelled = false;
    std::vector<ScheduledAction> m_scheduledActions;
};

} // namespace Application
