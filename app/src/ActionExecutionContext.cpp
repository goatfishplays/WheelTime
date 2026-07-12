/**
 * @file ActionExecutionContext.cpp
 * @brief ActionExecutionContext definitions.
 */

#include "App/ActionExecutionContext.hpp"

#include <atomic>
#include <stdexcept>

namespace Application
{
namespace
{

std::atomic<uint64_t> g_nextActionId{1};

} // namespace

ActionExecutionContext::ActionExecutionContext(std::unique_ptr<Action> action)
    : m_action{std::move(action)}
    , m_actionId{g_nextActionId.fetch_add(1, std::memory_order_relaxed)}
{
    if (!m_action)
    {
        throw std::invalid_argument("ActionExecutionContext requires a non-null Action");
    }
}

uint64_t ActionExecutionContext::actionId() const noexcept
{
    return m_actionId;
}

uint32_t ActionExecutionContext::channel() const noexcept
{
    return m_action->channel();
}

bool ActionExecutionContext::isCancelled() const noexcept
{
    return m_cancelled;
}

void ActionExecutionContext::cancel() noexcept
{
    m_cancelled = true;
}

Action &ActionExecutionContext::action() noexcept
{
    return *m_action;
}

const Action &ActionExecutionContext::action() const noexcept
{
    return *m_action;
}

ActionItem *ActionExecutionContext::currentItem() noexcept
{
    if (finished())
    {
        return nullptr;
    }
    return m_action->items()[m_currentIndex].get();
}

const ActionItem *ActionExecutionContext::currentItem() const noexcept
{
    if (finished())
    {
        return nullptr;
    }
    return m_action->items()[m_currentIndex].get();
}

void ActionExecutionContext::advance() noexcept
{
    if (!finished())
    {
        ++m_currentIndex;
    }
}

bool ActionExecutionContext::finished() const noexcept
{
    return m_currentIndex >= m_action->items().size();
}

void ActionExecutionContext::scheduleAction(std::unique_ptr<Action> action,
                                           std::chrono::steady_clock::time_point wakeTime)
{
    if (!action)
    {
        return;
    }

    m_scheduledActions.push_back(ScheduledAction{std::move(action), wakeTime});
}

const std::vector<ScheduledAction> &ActionExecutionContext::scheduledActions() const noexcept
{
    return m_scheduledActions;
}

std::vector<ScheduledAction> ActionExecutionContext::takeScheduledActions() noexcept
{
    std::vector<ScheduledAction> pending;
    pending.swap(m_scheduledActions);
    return pending;
}

void ActionExecutionContext::clearScheduledActions() noexcept
{
    m_scheduledActions.clear();
}

} // namespace Application
