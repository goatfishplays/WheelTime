/**
 * @file NthRecent.cpp
 * @brief AI_nthRecent definitions.
 */

#include "App/ActionItems/NthRecent.hpp"

#include <iostream>

namespace Application
{

std::unique_ptr<ActionItem> AI_nthRecent::clone() const
{
    return std::make_unique<AI_nthRecent>(*this);
}

ActionItemKind AI_nthRecent::kind() const
{
    return ActionItemKind::NthRecent;
}

ExecuteResult AI_nthRecent::execute(ActionExecutionContext & /*context*/)
{
    // When action-history tracking exists:
    // Action* recent = App::getInstance().nthRecentAction(n);
    // if (recent != nullptr) {
    //     context.scheduleAction(std::make_unique<Action>(*recent),
    //                            std::chrono::steady_clock::now());
    // }
    std::cerr << "[AI_nthRecent] would run nth recent action n=" << n << '\n';
    return ExecuteResult::Continue();
}

} // namespace Application
