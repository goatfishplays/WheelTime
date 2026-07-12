/**
 * @file Close.cpp
 * @brief AI_Close definitions.
 */

#include "App/ActionItems/Close.hpp"

#include "App/App.hpp"

namespace Application
{

std::unique_ptr<ActionItem> AI_Close::clone() const
{
    return std::make_unique<AI_Close>(*this);
}

ActionItemKind AI_Close::kind() const
{
    return ActionItemKind::Close;
}

ExecuteResult AI_Close::execute(ActionExecutionContext & /*context*/)
{
    App::getInstance().hideGui();
    return ExecuteResult::Continue();
}

} // namespace Application
