/**
 * @file Scheduler.cpp
 * @brief Scheduler definitions.
 */

#include "App/Scheduler.hpp"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace Application
{

Scheduler::Scheduler(std::size_t workerCount)
    : m_workers{
          workerCount,
          [this](WorkerResult result)
          {
              m_mailbox.push(SchedulerEvent{std::move(result)});
          }}
{
    m_thread = std::jthread([this] { threadMain(); });
}

Scheduler::~Scheduler()
{
    stop();
}

uint64_t Scheduler::allocateActionId()
{
    return m_nextActionId.fetch_add(1, std::memory_order_relaxed);
}

uint64_t Scheduler::submit(std::unique_ptr<Action> action)
{
    if (!action)
    {
        return 0;
    }

    const uint64_t actionId = allocateActionId();
    SubmitRequest request;
    request.action = std::move(action);
    request.earliestStart = {};
    request.actionId = actionId;
    m_mailbox.push(SchedulerEvent{std::move(request)});
    return actionId;
}

uint64_t Scheduler::submit(const Action &action)
{
    return submit(std::make_unique<Action>(action));
}

void Scheduler::cancelAction(uint64_t actionId)
{
    if (actionId == 0)
    {
        return;
    }
    m_mailbox.push(SchedulerEvent{CancelActionRequest{actionId}});
}

void Scheduler::cancelChannel(uint32_t channel)
{
    m_mailbox.push(SchedulerEvent{CancelChannelRequest{channel}});
}

void Scheduler::cancelAll()
{
    m_mailbox.push(SchedulerEvent{CancelAllRequest{}});
}

void Scheduler::stop()
{
    m_workers.stop();
    m_mailbox.stop();
    m_thread = std::jthread{};
}

void Scheduler::threadMain()
{
    SchedulerEvent event;
    for (;;)
    {
        processDueDelays();

        const auto wake = m_delayQueue.nextWakeTime();
        QueueWaitStatus status = QueueWaitStatus::Stopped;

        if (wake.has_value())
        {
            status = m_mailbox.popWaitUntil(event, *wake);
            if (status == QueueWaitStatus::Timeout)
            {
                continue;
            }
        }
        else if (m_mailbox.pop(event))
        {
            status = QueueWaitStatus::Item;
        }

        if (status != QueueWaitStatus::Item)
        {
            break;
        }

        std::visit(
            [this](auto &payload)
            {
                using T = std::decay_t<decltype(payload)>;
                if constexpr (std::is_same_v<T, SubmitRequest>)
                {
                    handleSubmit(std::move(payload));
                }
                else if constexpr (std::is_same_v<T, WorkerResult>)
                {
                    handleResult(std::move(payload));
                }
                else if constexpr (std::is_same_v<T, CancelActionRequest>)
                {
                    handleCancelAction(payload.actionId);
                }
                else if constexpr (std::is_same_v<T, CancelChannelRequest>)
                {
                    handleCancelChannel(payload.channel);
                }
                else if constexpr (std::is_same_v<T, CancelAllRequest>)
                {
                    handleCancelAll();
                }
            },
            event);
    }
}

void Scheduler::registerContext(const ActionExecutionContext &context)
{
    m_cancelFlags[context.actionId()] = context.cancelFlag();
    m_cancelableById[context.actionId()] = context.isCancelable();
    m_logicalChannelById[context.actionId()] = context.channel();
}

void Scheduler::unregisterContext(uint64_t actionId)
{
    m_cancelFlags.erase(actionId);
    m_cancelableById.erase(actionId);
    m_logicalChannelById.erase(actionId);
    m_flushActions.erase(actionId);
    m_linkedDelayedByParent.erase(actionId);

    for (auto &[parentId, children] : m_linkedDelayedByParent)
    {
        (void)parentId;
        children.erase(std::remove(children.begin(), children.end(), actionId), children.end());
    }
}

bool Scheduler::isCancelableAction(uint64_t actionId) const
{
    const auto it = m_cancelableById.find(actionId);
    if (it == m_cancelableById.end())
    {
        return true;
    }
    return it->second;
}

void Scheduler::applyDispatch(ChannelDispatch dispatch)
{
    switch (dispatch.type)
    {
    case ChannelDispatch::Type::None:
        break;
    case ChannelDispatch::Type::RunNow:
        if (dispatch.context)
        {
            m_workers.submit(std::move(dispatch.context));
        }
        break;
    case ChannelDispatch::Type::ParkUntil:
        if (dispatch.context)
        {
            m_delayQueue.push(dispatch.wakeTime, std::move(dispatch.context));
        }
        break;
    }
}

void Scheduler::handleSubmit(SubmitRequest request)
{
    if (!request.action)
    {
        return;
    }

    const uint64_t actionId =
        request.actionId != 0 ? request.actionId : allocateActionId();

    if (request.removeIfParentCancelled && request.parentActionId != 0)
    {
        m_linkedDelayedByParent[request.parentActionId].push_back(actionId);
    }

    auto context = std::make_unique<ActionExecutionContext>(std::move(request.action), actionId);
    registerContext(*context);
    applyDispatch(m_channels.admit(
        std::move(context),
        request.earliestStart,
        std::chrono::steady_clock::now()));
}

void Scheduler::ingestCancelFlushes(ActionExecutionContext &context)
{
    auto flushes = context.takeCancelFlushes();
    if (flushes.empty())
    {
        return;
    }

    auto &slot = m_flushActions[context.actionId()];
    for (std::unique_ptr<Action> &flush : flushes)
    {
        slot.push_back(std::move(flush));
    }
}

void Scheduler::ingestScheduledActions(ActionExecutionContext &context)
{
    for (ScheduledAction &scheduled : context.takeScheduledActions())
    {
        if (!scheduled.action)
        {
            continue;
        }

        SubmitRequest request;
        request.action = std::move(scheduled.action);
        request.earliestStart = scheduled.wakeTime;
        request.actionId = allocateActionId();
        request.parentActionId = scheduled.parentActionId != 0 ? scheduled.parentActionId
                                                              : context.actionId();
        request.removeIfParentCancelled = scheduled.removeIfParentCancelled;
        handleSubmit(std::move(request));
    }
}

void Scheduler::flushCleanups(uint64_t actionId)
{
    auto it = m_flushActions.find(actionId);
    if (it == m_flushActions.end())
    {
        return;
    }

    std::vector<std::unique_ptr<Action>> flushes = std::move(it->second);
    m_flushActions.erase(it);

    const auto now = std::chrono::steady_clock::now();
    for (std::unique_ptr<Action> &flush : flushes)
    {
        if (!flush)
        {
            continue;
        }
        flush->setCancelable(false);
        SubmitRequest request;
        request.action = std::move(flush);
        request.earliestStart = now;
        request.actionId = allocateActionId();
        handleSubmit(std::move(request));
    }
}

void Scheduler::discardContext(std::unique_ptr<ActionExecutionContext> context, bool releaseChannel)
{
    if (!context)
    {
        return;
    }

    const uint64_t actionId = context->actionId();
    const auto channelKey = m_channels.channelFor(actionId);
    m_channels.unbind(actionId);
    unregisterContext(actionId);

    if (releaseChannel && channelKey.has_value())
    {
        applyDispatch(m_channels.releaseAndTakeNext(
            *channelKey, std::chrono::steady_clock::now()));
    }
}

void Scheduler::handleCancelAction(uint64_t actionId)
{
    if (!isCancelableAction(actionId))
    {
        return;
    }

    flushCleanups(actionId);

    // Discard linked delayed cleanups (flush already released keys immediately).
    std::vector<uint64_t> linked;
    if (auto it = m_linkedDelayedByParent.find(actionId); it != m_linkedDelayedByParent.end())
    {
        linked = std::move(it->second);
        m_linkedDelayedByParent.erase(it);
    }
    for (uint64_t childId : linked)
    {
        auto waiting = m_channels.removeWaitingIf(
            [childId](const ActionExecutionContext &ctx) { return ctx.actionId() == childId; });
        for (auto &ctx : waiting)
        {
            discardContext(std::move(ctx), false);
        }

        auto delayed = m_delayQueue.removeIf(
            [childId](const ActionExecutionContext &ctx) { return ctx.actionId() == childId; });
        for (auto &ctx : delayed)
        {
            discardContext(std::move(ctx), true);
        }

        // If somehow still only flagged as in-flight uncancelable, leave it;
        // linked cleanups are normally delayed/queued.
        unregisterContext(childId);
        m_channels.unbind(childId);
    }

    auto waiting = m_channels.removeWaitingIf(
        [actionId](const ActionExecutionContext &ctx) { return ctx.actionId() == actionId; });
    if (!waiting.empty())
    {
        for (auto &ctx : waiting)
        {
            discardContext(std::move(ctx), false);
        }
        return;
    }

    auto delayed = m_delayQueue.removeIf(
        [actionId](const ActionExecutionContext &ctx) { return ctx.actionId() == actionId; });
    if (!delayed.empty())
    {
        for (auto &ctx : delayed)
        {
            discardContext(std::move(ctx), true);
        }
        return;
    }

    // Executing (or not yet admitted): set shared cancel flag.
    if (auto flagIt = m_cancelFlags.find(actionId); flagIt != m_cancelFlags.end() && flagIt->second)
    {
        flagIt->second->store(true, std::memory_order_release);
    }
}

void Scheduler::handleCancelChannel(uint32_t channel)
{
    std::vector<uint64_t> ids;
    for (const auto &[id, logical] : m_logicalChannelById)
    {
        if (logical == channel && isCancelableAction(id))
        {
            ids.push_back(id);
        }
    }
    for (uint64_t id : ids)
    {
        handleCancelAction(id);
    }
}

void Scheduler::handleCancelAll()
{
    std::vector<uint64_t> ids;
    for (const auto &[id, cancelable] : m_cancelableById)
    {
        if (cancelable)
        {
            ids.push_back(id);
        }
    }
    for (uint64_t id : ids)
    {
        handleCancelAction(id);
    }
}

void Scheduler::handleResult(WorkerResult result)
{
    if (!result.context)
    {
        return;
    }

    const uint64_t actionId = result.context->actionId();
    const auto channelKey = m_channels.channelFor(actionId);

    switch (result.status)
    {
    case WorkerResult::Status::Continue:
    {
        ingestCancelFlushes(*result.context);
        ingestScheduledActions(*result.context);
        if (!channelKey.has_value())
        {
            return;
        }
        // Channel stays busy; resume the same Action on a worker.
        m_workers.submit(std::move(result.context));
        break;
    }
    case WorkerResult::Status::Delayed:
    {
        ingestCancelFlushes(*result.context);
        ingestScheduledActions(*result.context);
        if (!channelKey.has_value())
        {
            return;
        }
        // Hold channel until wake — park on DelayQueue; do not release the channel.
        m_delayQueue.push(result.wakeTime, std::move(result.context));
        break;
    }
    case WorkerResult::Status::Completed:
    {
        ingestCancelFlushes(*result.context);
        ingestScheduledActions(*result.context);
        if (!channelKey.has_value())
        {
            unregisterContext(actionId);
            return;
        }
        m_channels.unbind(actionId);
        unregisterContext(actionId);
        applyDispatch(m_channels.releaseAndTakeNext(
            *channelKey, std::chrono::steady_clock::now()));
        break;
    }
    case WorkerResult::Status::Cancelled:
    {
        // Parent terminating: run flushes, do not admit pending schedules.
        ingestCancelFlushes(*result.context);
        flushCleanups(actionId);
        result.context->clearScheduledActions();
        if (!channelKey.has_value())
        {
            unregisterContext(actionId);
            return;
        }
        m_channels.unbind(actionId);
        unregisterContext(actionId);
        applyDispatch(m_channels.releaseAndTakeNext(
            *channelKey, std::chrono::steady_clock::now()));
        break;
    }
    }
}

void Scheduler::processDueDelays()
{
    for (std::unique_ptr<ActionExecutionContext> &context :
         m_delayQueue.popDue(std::chrono::steady_clock::now()))
    {
        if (context)
        {
            m_workers.submit(std::move(context));
        }
    }
}

} // namespace Application
