/**
 * @file ActionItem.cpp
 * @brief Base ActionItem definitions.
 */

#include "App/ActionItems/ActionItem.hpp"

#include <iostream>

namespace Application
{

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
