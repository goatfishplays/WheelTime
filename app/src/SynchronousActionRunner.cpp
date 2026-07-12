/**
 * @file SynchronousActionRunner.cpp
 * @brief SynchronousActionRunner definitions.
 */

#include "App/SynchronousActionRunner.hpp"

#include "App/ActionExecutionContext.hpp"
#include "App/ActionItems.hpp"
#include "App/ExecuteResult.hpp"

#include <chrono>
#include <thread>
#include <utility>
#include <vector>

namespace Application
{

void SynchronousActionRunner::run(const Action &action)
{
    run(std::make_unique<Action>(action));
}

void SynchronousActionRunner::run(std::unique_ptr<Action> action)
{
    if (!action)
    {
        return;
    }

    ActionExecutionContext context{std::move(action)};

    while (!context.finished() && !context.isCancelled())
    {
        ActionItem *item = context.currentItem();
        if (item == nullptr)
        {
            break;
        }

        const ExecuteResult result = item->execute(context);

        for (ScheduledAction &request : context.takeScheduledActions())
        {
            const auto now = std::chrono::steady_clock::now();
            if (request.wakeTime > now)
            {
                std::this_thread::sleep_until(request.wakeTime);
            }
            run(std::move(request.action));
        }

        if (result.type() == ExecuteResult::Type::DelayUntil)
        {
            const auto now = std::chrono::steady_clock::now();
            if (result.wakeTime() > now)
            {
                std::this_thread::sleep_until(result.wakeTime());
            }
        }

        context.advance();
    }
}

} // namespace Application
