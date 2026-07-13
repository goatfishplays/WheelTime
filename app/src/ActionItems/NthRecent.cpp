/**
 * @file NthRecent.cpp
 * @brief AI_nthRecent definitions.
 */

#include "App/ActionItems/NthRecent.hpp"

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/App.hpp"

#include <chrono>
#include <iostream>
#include <memory>

namespace Application
{

AI_nthRecent::AI_nthRecent(int n)
    : n{n}
{
}

std::unique_ptr<ActionItem> AI_nthRecent::clone() const
{
    return std::make_unique<AI_nthRecent>(*this);
}

ActionItemKind AI_nthRecent::kind() const
{
    return ActionItemKind::NthRecent;
}

ExecuteResult AI_nthRecent::execute(ActionExecutionContext &context)
{
    if (n < 1)
    {
        std::cerr << "[AI_nthRecent] ignored invalid n=" << n << " (expected >= 1)\n";
        return ExecuteResult::Continue();
    }

    Action *recent = App::getInstance().nthRecentAction(n);
    if (recent == nullptr)
    {
        std::cerr << "[AI_nthRecent] no history entry for n=" << n << '\n';
        return ExecuteResult::Continue();
    }

    std::cerr << "[AI_nthRecent] n=" << n << " -> actionId=" << recent->getId()
              << " name=" << recent->getName() << '\n';
    context.scheduleAction(
        std::make_unique<Action>(*recent),
        std::chrono::steady_clock::now());
    return ExecuteResult::Continue();
}

} // namespace Application
