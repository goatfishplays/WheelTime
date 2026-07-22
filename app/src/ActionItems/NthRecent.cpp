/**
 * @file NthRecent.cpp
 * @brief AI_NthRecent definitions.
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

AI_NthRecent::AI_NthRecent(int n)
    : n{n}
{
}

std::unique_ptr<ActionItem> AI_NthRecent::clone() const
{
    return std::make_unique<AI_NthRecent>(*this);
}

ActionItemKind AI_NthRecent::kind() const
{
    return ActionItemKind::NthRecent;
}

ExecuteResult AI_NthRecent::execute(ActionExecutionContext &context)
{
    if (n < 1)
    {
        std::cerr << "[AI_NthRecent] ignored invalid n=" << n << " (expected >= 1)\n";
        return ExecuteResult::Continue();
    }

    Action *recent = App::instance().nthRecentAction(n);
    if (recent == nullptr)
    {
        std::cerr << "[AI_NthRecent] no history entry for n=" << n << '\n';
        return ExecuteResult::Continue();
    }

    std::cerr << "[AI_NthRecent] n=" << n << " -> actionId=" << recent->id()
              << " name=" << recent->name() << '\n';
    context.scheduleAction(
        std::make_unique<Action>(*recent),
        std::chrono::steady_clock::now());
    return ExecuteResult::Continue();
}

} // namespace Application
