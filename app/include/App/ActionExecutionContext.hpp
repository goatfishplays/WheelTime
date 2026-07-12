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
 */
struct ScheduledAction
{
    std::unique_ptr<Action> action;
    std::chrono::steady_clock::time_point wakeTime;
};

/**
 * @brief Owns an Action and tracks progress while workers run its items.
 *
 * ActionItems record scheduler requests here instead of talking to the
 * scheduler directly. Contains no scheduling policy of its own.
 */
class ActionExecutionContext
{
public:
    explicit ActionExecutionContext(std::unique_ptr<Action> action);

    ActionExecutionContext(const ActionExecutionContext &) = delete;
    ActionExecutionContext &operator=(const ActionExecutionContext &) = delete;

    ActionExecutionContext(ActionExecutionContext &&) noexcept = default;
    ActionExecutionContext &operator=(ActionExecutionContext &&) noexcept = default;

    ~ActionExecutionContext() = default;

    /// @brief Runtime instance id (distinct from Action's config/library string id).
    [[nodiscard]] uint64_t actionId() const noexcept;

    [[nodiscard]] uint32_t channel() const noexcept;

    [[nodiscard]] bool isCancelled() const noexcept;

    /// @brief Marks this execution cancelled. Takes effect between ActionItems.
    void cancel() noexcept;

    [[nodiscard]] Action &action() noexcept;
    [[nodiscard]] const Action &action() const noexcept;

    [[nodiscard]] ActionItem *currentItem() noexcept;
    [[nodiscard]] const ActionItem *currentItem() const noexcept;

    /// @brief Advances to the next item without executing it.
    void advance() noexcept;

    [[nodiscard]] bool finished() const noexcept;

    /**
     * @brief Records an Action for the scheduler to pick up after execute() returns.
     */
    void scheduleAction(std::unique_ptr<Action> action,
                        std::chrono::steady_clock::time_point wakeTime);

    [[nodiscard]] const std::vector<ScheduledAction> &scheduledActions() const noexcept;

    /// @brief Moves out all pending schedule requests and clears the list.
    [[nodiscard]] std::vector<ScheduledAction> takeScheduledActions() noexcept;

    void clearScheduledActions() noexcept;

private:
    std::unique_ptr<Action> m_action;
    uint64_t m_actionId = 0;
    size_t m_currentIndex = 0;
    bool m_cancelled = false;
    std::vector<ScheduledAction> m_scheduledActions;
};

} // namespace Application
