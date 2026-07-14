/**
 * @file LaunchApp.cpp
 * @brief AI_LaunchApp definitions.
 */

#include "App/ActionItems/LaunchApp.hpp"

#include "App/App.hpp"

#include <iostream>

namespace Application
{

AI_LaunchApp::AI_LaunchApp(std::string presetId, std::string customTarget)
    : presetId{std::move(presetId)}
    , customTarget{std::move(customTarget)}
{
}

std::unique_ptr<ActionItem> AI_LaunchApp::clone() const
{
    return std::make_unique<AI_LaunchApp>(*this);
}

ActionItemKind AI_LaunchApp::kind() const
{
    return ActionItemKind::LaunchApp;
}

ExecuteResult AI_LaunchApp::execute(ActionExecutionContext & /*context*/)
{
    if (presetId == "custom")
    {
        if (customTarget.empty())
        {
            std::cerr << "[AI_LaunchApp] custom target empty; skipping\n";
            return ExecuteResult::Continue();
        }
        std::cerr << "[AI_LaunchApp] custom target='" << customTarget << "'\n";
        App::getInstance().executor.executeScript(customTarget);
        return ExecuteResult::Continue();
    }

    std::cerr << "[AI_LaunchApp] preset='" << presetId << "'\n";
    App::getInstance().executor.executeLaunchPreset(presetId);
    return ExecuteResult::Continue();
}

} // namespace Application
