/**
 * @file NthFrequent.cpp
 * @brief AI_NthFrequent definitions.
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

AI_NthFrequent::AI_NthFrequent(int n)
    : n{n}
{
}

std::unique_ptr<ActionItem> AI_NthFrequent::clone() const
{
    return std::make_unique<AI_NthFrequent>(*this);
}

ActionItemKind AI_NthFrequent::kind() const
{
    return ActionItemKind::NthFrequent;
}

ExecuteResult AI_NthFrequent::execute(ActionExecutionContext &context)
{
    if (n < 1)
    {
        std::cerr << "[AI_NthFrequent] ignored invalid n=" << n << " (expected >= 1)\n";
        return ExecuteResult::Continue();
    }

    Action *frequent = App::instance().nthFrequentAction(n);
    if (frequent == nullptr)
    {
        std::cerr << "[AI_NthFrequent] no history entry for n=" << n << '\n';
        return ExecuteResult::Continue();
    }

    std::cerr << "[AI_NthFrequent] n=" << n << " -> actionId=" << frequent->id()
              << " name=" << frequent->name() << '\n';
    context.scheduleAction(
        std::make_unique<Action>(*frequent),
        std::chrono::steady_clock::now());
    return ExecuteResult::Continue();
}

} // namespace Application
