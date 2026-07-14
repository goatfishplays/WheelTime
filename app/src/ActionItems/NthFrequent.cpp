/**
 * @file NthFrequent.cpp
 * @brief AI_nthFrequent definitions.
 */

#include "App/ActionItems/NthFrequent.hpp"

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/App.hpp"

#include <chrono>
#include <iostream>
#include <memory>

namespace Application
{

AI_nthFrequent::AI_nthFrequent(int n)
    : n{n}
{
}

std::unique_ptr<ActionItem> AI_nthFrequent::clone() const
{
    return std::make_unique<AI_nthFrequent>(*this);
}

ActionItemKind AI_nthFrequent::kind() const
{
    return ActionItemKind::NthFrequent;
}

ExecuteResult AI_nthFrequent::execute(ActionExecutionContext &context)
{
    if (n < 1)
    {
        std::cerr << "[AI_nthFrequent] ignored invalid n=" << n << " (expected >= 1)\n";
        return ExecuteResult::Continue();
    }

    Action *frequent = App::getInstance().nthFrequentAction(n);
    if (frequent == nullptr)
    {
        std::cerr << "[AI_nthFrequent] no history entry for n=" << n << '\n';
        return ExecuteResult::Continue();
    }

    std::cerr << "[AI_nthFrequent] n=" << n << " -> actionId=" << frequent->getId()
              << " name=" << frequent->getName() << '\n';
    context.scheduleAction(
        std::make_unique<Action>(*frequent),
        std::chrono::steady_clock::now());
    return ExecuteResult::Continue();
}

} // namespace Application
