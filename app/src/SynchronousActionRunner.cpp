/**
 * @file SynchronousActionRunner.cpp
 * @brief SynchronousActionRunner definitions.
 */

#include "App/SynchronousActionRunner.hpp"

#include "App/ActionExecutionContext.hpp"
#include "App/ActionItems.hpp"
#include "App/ExecuteResult.hpp"

#include <chrono>
#include <iostream>
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

        const Action &owned = context.action();
        if (context.currentIndex() == 0)
        {
            std::cerr << "[Action] run runtimeId=" << context.actionId()
                      << " id=" << owned.getId() << " name=" << owned.getName()
                      << " channel=" << owned.channel()
                      << " cancelable=" << (owned.cancelable() ? "true" : "false") << '\n';
        }
        std::cerr << "[ActionItem] execute kind=" << actionItemKindName(item->kind())
                  << " index=" << context.currentIndex()
                  << " actionId=" << owned.getId()
                  << " runtimeId=" << context.actionId() << '\n';

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

        // Best-effort: without a live Scheduler, only local cancel can apply.
        for (const PendingCancelRequest &request : context.takeCancelRequests())
        {
            switch (request.level)
            {
            case PendingCancelRequest::Level::MostRecent:
                // No peer Actions exist in the synchronous runner; nothing to cancel.
                break;
            case PendingCancelRequest::Level::Channel:
                if (request.channel == context.channel())
                {
                    context.cancel();
                }
                break;
            case PendingCancelRequest::Level::All:
                context.cancel();
                break;
            }
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
