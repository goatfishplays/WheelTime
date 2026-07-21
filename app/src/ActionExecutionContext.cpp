/**
 * @file ActionExecutionContext.cpp
 * @brief ActionExecutionContext definitions.
 */

#include "App/ActionExecutionContext.hpp"

#include <atomic>
#include <stdexcept>
#include <utility>

namespace Application
{
namespace
{

std::atomic<uint64_t> g_nextActionId{1};

} // namespace

ActionExecutionContext::ActionExecutionContext(std::unique_ptr<Action> action, uint64_t actionId)
    : m_action{std::move(action)}
    , m_actionId{actionId == 0 ? g_nextActionId.fetch_add(1, std::memory_order_relaxed) : actionId}
    , m_cancelFlag{std::make_shared<std::atomic<bool>>(false)}
{
    if (!m_action)
    {
        throw std::invalid_argument("ActionExecutionContext requires a non-null Action");
    }
    if (actionId != 0)
    {
        // Keep the global counter ahead of any preassigned ids.
        uint64_t observed = g_nextActionId.load(std::memory_order_relaxed);
        while (observed <= actionId
               && !g_nextActionId.compare_exchange_weak(
                   observed, actionId + 1, std::memory_order_relaxed))
        {
        }
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

bool ActionExecutionContext::isCancelable() const noexcept
{
    return m_action->isCancelable();
}

bool ActionExecutionContext::isCancelled() const noexcept
{
    return m_cancelFlag && m_cancelFlag->load(std::memory_order_acquire);
}

void ActionExecutionContext::cancel() noexcept
{
    if (m_cancelFlag)
    {
        m_cancelFlag->store(true, std::memory_order_release);
    }
}

std::shared_ptr<std::atomic<bool>> ActionExecutionContext::cancelFlag() const noexcept
{
    return m_cancelFlag;
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

size_t ActionExecutionContext::currentIndex() const noexcept
{
    return m_currentIndex;
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
                                           std::chrono::steady_clock::time_point wakeTime,
                                           bool removeIfParentCancelled)
{
    if (!action)
    {
        return;
    }

    m_scheduledActions.push_back(ScheduledAction{
        std::move(action),
        wakeTime,
        m_actionId,
        removeIfParentCancelled});
}

void ActionExecutionContext::setCancelFlush(std::unique_ptr<Action> action)
{
    if (!action)
    {
        return;
    }
    m_cancelFlushes.push_back(std::move(action));
}

void ActionExecutionContext::requestCancelMostRecent(uint32_t channel)
{
    m_cancelRequests.push_back(PendingCancelRequest{
        PendingCancelRequest::Level::MostRecent,
        m_actionId,
        channel});
}

void ActionExecutionContext::requestCancelChannel(uint32_t channel)
{
    m_cancelRequests.push_back(
        PendingCancelRequest{PendingCancelRequest::Level::Channel, 0, channel});
}

void ActionExecutionContext::requestCancelAll()
{
    m_cancelRequests.push_back(
        PendingCancelRequest{PendingCancelRequest::Level::All, 0, 0});
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

std::vector<std::unique_ptr<Action>> ActionExecutionContext::takeCancelFlushes() noexcept
{
    std::vector<std::unique_ptr<Action>> pending;
    pending.swap(m_cancelFlushes);
    return pending;
}

std::vector<PendingCancelRequest> ActionExecutionContext::takeCancelRequests() noexcept
{
    std::vector<PendingCancelRequest> pending;
    pending.swap(m_cancelRequests);
    return pending;
}

bool ActionExecutionContext::hasPendingSchedulerRequests() const noexcept
{
    return !m_scheduledActions.empty() || !m_cancelFlushes.empty() || !m_cancelRequests.empty();
}

void ActionExecutionContext::markCancelFlush() noexcept
{
    m_isCancelFlush = true;
}

bool ActionExecutionContext::isCancelFlush() const noexcept
{
    return m_isCancelFlush;
}

} // namespace Application
