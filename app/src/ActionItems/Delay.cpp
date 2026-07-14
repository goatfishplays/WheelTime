/**
 * @file Delay.cpp
 * @brief AI_Delay definitions.
 */

#include "App/ActionItems/Delay.hpp"

#include <chrono>
#include <iostream>

namespace Application
{

AI_Delay::AI_Delay(int durationMs)
    : duration{durationMs}
{
}

std::unique_ptr<ActionItem> AI_Delay::clone() const
{
    return std::make_unique<AI_Delay>(*this);
}

ActionItemKind AI_Delay::kind() const
{
    return ActionItemKind::Delay;
}

ExecuteResult AI_Delay::execute(ActionExecutionContext & /*context*/)
{
    std::cerr << "[AI_Delay] durationMs=" << duration << '\n';
    return ExecuteResult::DelayUntil(
        std::chrono::steady_clock::now() + std::chrono::milliseconds(duration));
}

} // namespace Application
