/**
 * @file ActionItem.cpp
 * @brief Base ActionItem definitions.
 */

#include "App/ActionItems/ActionItem.hpp"

#include <iostream>

namespace Application
{

const char *actionItemKindName(ActionItemKind kind) noexcept
{
    switch (kind)
    {
    case ActionItemKind::Base:
        return "base";
    case ActionItemKind::LaunchApp:
        return "launch_app";
    case ActionItemKind::Script:
        return "script";
    case ActionItemKind::Delay:
        return "delay";
    case ActionItemKind::Menu:
        return "menu";
    case ActionItemKind::Close:
        return "close";
    case ActionItemKind::Keystroke:
        return "keystroke";
    case ActionItemKind::KeyRelease:
        return "key_release";
    case ActionItemKind::Socket:
        return "socket";
    case ActionItemKind::NthRecent:
        return "nth_recent";
    case ActionItemKind::NthFrequent:
        return "nth_frequent";
    case ActionItemKind::MouseMove:
        return "mouse_move";
    case ActionItemKind::MouseButton:
        return "mouse_button";
    case ActionItemKind::MouseButtonRelease:
        return "mouse_button_release";
    case ActionItemKind::Cancel:
        return "cancel";
    case ActionItemKind::Search:
        return "search";
    }
    return "unknown";
}

std::unique_ptr<ActionItem> ActionItem::clone() const
{
    return std::make_unique<ActionItem>(*this);
}

ActionItemKind ActionItem::kind() const
{
    return ActionItemKind::Base;
}

ExecuteResult ActionItem::execute(ActionExecutionContext & /*context*/)
{
    std::cerr << "ActionItem base execute() called; nothing to do.\n";
    return ExecuteResult::Continue();
}

} // namespace Application
