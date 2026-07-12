/**
 * @file NthFrequent.cpp
 * @brief AI_nthFrequent definitions.
 */

#include "App/ActionItems/NthFrequent.hpp"

#include <iostream>

namespace Application
{

std::unique_ptr<ActionItem> AI_nthFrequent::clone() const
{
    return std::make_unique<AI_nthFrequent>(*this);
}

ActionItemKind AI_nthFrequent::kind() const
{
    return ActionItemKind::NthFrequent;
}

ExecuteResult AI_nthFrequent::execute(ActionExecutionContext & /*context*/)
{
    // When usage-frequency tracking exists:
    // Action* frequent = App::getInstance().nthFrequentAction(n);
    // if (frequent != nullptr) {
    //     context.scheduleAction(std::make_unique<Action>(*frequent),
    //                            std::chrono::steady_clock::now());
    // }
    std::cerr << "[AI_nthFrequent] would run nth frequent action n=" << n << '\n';
    return ExecuteResult::Continue();
}

} // namespace Application
