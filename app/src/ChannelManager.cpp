/**
 * @file ChannelManager.cpp
 * @brief ChannelManager definitions.
 */

#include "App/ChannelManager.hpp"

#include <utility>

namespace Application
{

uint32_t ChannelManager::resolveKey(const Action &action)
{
    if (action.channel() == 0)
    {
        return m_nextPrivateChannel++;
    }
    return action.channel();
}

void ChannelManager::bind(uint64_t actionId, uint32_t channelKey)
{
    m_channelByActionId[actionId] = channelKey;
}

std::optional<uint32_t> ChannelManager::channelFor(uint64_t actionId) const
{
    const auto it = m_channelByActionId.find(actionId);
    if (it == m_channelByActionId.end())
    {
        return std::nullopt;
    }
    return it->second;
}

void ChannelManager::unbind(uint64_t actionId)
{
    m_channelByActionId.erase(actionId);
}

bool ChannelManager::isBusy(uint32_t channelKey) const
{
    const auto it = m_channels.find(channelKey);
    return it != m_channels.end() && it->second.busy;
}

ChannelDispatch ChannelManager::takeNextLocked(
    ChannelState &channel,
    std::chrono::steady_clock::time_point now)
{
    if (channel.busy || channel.waiting.empty())
    {
        return {};
    }

    WaitingEntry entry = std::move(channel.waiting.front());
    channel.waiting.pop();
    channel.busy = true;

    ChannelDispatch dispatch;
    dispatch.context = std::move(entry.context);

    if (entry.earliestStart > now)
    {
        dispatch.type = ChannelDispatch::Type::ParkUntil;
        dispatch.wakeTime = entry.earliestStart;
        return dispatch;
    }

    dispatch.type = ChannelDispatch::Type::RunNow;
    return dispatch;
}

ChannelDispatch ChannelManager::admit(
    std::unique_ptr<ActionExecutionContext> context,
    std::chrono::steady_clock::time_point earliestStart,
    std::chrono::steady_clock::time_point now)
{
    if (!context)
    {
        return {};
    }

    const uint32_t channelKey = resolveKey(context->action());
    bind(context->actionId(), channelKey);

    ChannelState &channel = m_channels[channelKey];
    if (channel.busy)
    {
        WaitingEntry entry;
        entry.context = std::move(context);
        entry.earliestStart = earliestStart;
        channel.waiting.push(std::move(entry));
        return {};
    }

    channel.busy = true;

    ChannelDispatch dispatch;
    dispatch.context = std::move(context);

    if (earliestStart > now)
    {
        dispatch.type = ChannelDispatch::Type::ParkUntil;
        dispatch.wakeTime = earliestStart;
        return dispatch;
    }

    dispatch.type = ChannelDispatch::Type::RunNow;
    return dispatch;
}

ChannelDispatch ChannelManager::releaseAndTakeNext(
    uint32_t channelKey,
    std::chrono::steady_clock::time_point now)
{
    ChannelState &channel = m_channels[channelKey];
    channel.busy = false;
    return takeNextLocked(channel, now);
}

} // namespace Application
