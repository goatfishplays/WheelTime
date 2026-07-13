/**
 * @file Cancel.cpp
 * @brief AI_Cancel definitions.
 */

#include "App/ActionItems/Cancel.hpp"

#include "App/ActionExecutionContext.hpp"

#include <iostream>

namespace Application
{
namespace
{

const char *cancelLevelName(CancelLevel level) noexcept
{
    switch (level)
    {
    case CancelLevel::Latest:
        return "latest";
    case CancelLevel::Channel:
        return "channel";
    case CancelLevel::All:
        return "all";
    }
    return "unknown";
}

} // namespace

AI_Cancel::AI_Cancel(CancelLevel level, uint32_t channel)
    : level{level}
    , channel{channel}
{
}

std::unique_ptr<ActionItem> AI_Cancel::clone() const
{
    return std::make_unique<AI_Cancel>(*this);
}

ActionItemKind AI_Cancel::kind() const
{
    return ActionItemKind::Cancel;
}

ExecuteResult AI_Cancel::execute(ActionExecutionContext &context)
{
    std::cerr << "[AI_Cancel] execute level=" << cancelLevelName(level)
              << " channel=" << channel
              << " requesterRuntimeId=" << context.actionId() << '\n';

    switch (level)
    {
    case CancelLevel::Latest:
        context.requestCancelMostRecent(channel);
        break;
    case CancelLevel::Channel:
        context.requestCancelChannel(channel);
        break;
    case CancelLevel::All:
        context.requestCancelAll();
        break;
    }
    return ExecuteResult::Continue();
}

} // namespace Application
