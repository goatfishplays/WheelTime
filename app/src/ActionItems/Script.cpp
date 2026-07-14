/**
 * @file Script.cpp
 * @brief AI_Script definitions.
 */

#include "App/ActionItems/Script.hpp"

#include "App/App.hpp"

#include <iostream>

namespace Application
{

AI_Script::AI_Script(std::string filepath)
    : filepath{std::move(filepath)}
{
}

std::unique_ptr<ActionItem> AI_Script::clone() const
{
    return std::make_unique<AI_Script>(*this);
}

ActionItemKind AI_Script::kind() const
{
    return ActionItemKind::Script;
}

ExecuteResult AI_Script::execute(ActionExecutionContext & /*context*/)
{
    std::cerr << "[AI_Script] path='" << filepath << "'\n";
    App::getInstance().executor.executeScript(filepath);
    return ExecuteResult::Continue();
}

} // namespace Application
