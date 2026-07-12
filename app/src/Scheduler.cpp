/**
 * @file Scheduler.cpp
 * @brief Scheduler definitions.
 */

#include "App/Scheduler.hpp"

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

void Scheduler::submit(std::unique_ptr<Action> action)
{
    if (!action)
    {
        return;
    }

    SubmitRequest request;
    request.action = std::move(action);
    request.earliestStart = {};
    m_mailbox.push(SchedulerEvent{std::move(request)});
}

void Scheduler::submit(const Action &action)
{
    submit(std::make_unique<Action>(action));
}

void Scheduler::stop()
{
    // Let in-flight workers deliver final results before closing the mailbox.
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
            // Sleep until: new mailbox event, earliest delay wake, or stop.
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
                else
                {
                    handleResult(std::move(payload));
                }
            },
            event);
    }
}

void Scheduler::handleSubmit(SubmitRequest request)
{
    if (!request.action)
    {
        return;
    }

    auto context = std::make_unique<ActionExecutionContext>(std::move(request.action));
    const uint32_t channelKey = resolveChannelKey(context->action());
    m_channelByActionId[context->actionId()] = channelKey;

    ChannelState &channel = m_channels[channelKey];
    const auto now = std::chrono::steady_clock::now();

    if (channel.busy)
    {
        channel.waiting.push(std::move(context));
        return;
    }

    channel.busy = true;

    if (request.earliestStart > now)
    {
        m_delayQueue.push(request.earliestStart, std::move(context));
        return;
    }

    m_workers.submit(std::move(context));
}

void Scheduler::tryDispatch(uint32_t channelKey)
{
    ChannelState &channel = m_channels[channelKey];
    if (channel.busy || channel.waiting.empty())
    {
        return;
    }

    channel.busy = true;
    auto context = std::move(channel.waiting.front());
    channel.waiting.pop();
    m_workers.submit(std::move(context));
}

uint32_t Scheduler::resolveChannelKey(const Action &action)
{
    if (action.channel() == 0)
    {
        return m_nextPrivateChannel++;
    }
    return action.channel();
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
        handleSubmit(std::move(request));
    }
}

void Scheduler::handleResult(WorkerResult result)
{
    if (!result.context)
    {
        return;
    }

    ingestScheduledActions(*result.context);

    const uint64_t actionId = result.context->actionId();
    const auto channelIt = m_channelByActionId.find(actionId);
    if (channelIt == m_channelByActionId.end())
    {
        return;
    }
    const uint32_t channelKey = channelIt->second;

    switch (result.status)
    {
    case WorkerResult::Status::Delayed:
    {
        // Hold channel until wake — park on DelayQueue; do not dispatch waiters.
        m_delayQueue.push(result.wakeTime, std::move(result.context));
        break;
    }
    case WorkerResult::Status::Completed:
    case WorkerResult::Status::Cancelled:
    {
        m_channelByActionId.erase(channelIt);
        m_channels[channelKey].busy = false;
        tryDispatch(channelKey);
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
