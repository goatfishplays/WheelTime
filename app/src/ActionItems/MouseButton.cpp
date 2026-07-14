/**
 * @file MouseButton.cpp
 * @brief AI_MouseButton / AI_MouseButtonRelease definitions.
 */

#include "App/ActionItems/MouseButton.hpp"

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/ActionItems/Delay.hpp"
#include "App/App.hpp"

#include <chrono>
#include <iostream>
#include <vector>

namespace Application
{
namespace
{

const char *mouseButtonName(int button) noexcept
{
    switch (button)
    {
    case 0:
        return "left";
    case 1:
        return "right";
    case 2:
        return "middle";
    default:
        return "unknown";
    }
}

std::unique_ptr<Action> makeMouseButtonReleaseAction(int button, int modifiers)
{
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<AI_MouseButtonRelease>(button, modifiers));
    auto action =
        std::make_unique<Action>(std::move(items), "mouse-release", "", "mouse-release", 0);
    action->setCancelable(false);
    return action;
}

} // namespace

AI_MouseButton::AI_MouseButton(int button, float holdDuration, bool proceed, int modifiers)
    : button{button}
    , modifiers{modifiers}
    , holdDuration{holdDuration}
    , proceed{proceed}
{
}

std::unique_ptr<ActionItem> AI_MouseButton::clone() const
{
    return std::make_unique<AI_MouseButton>(*this);
}

ActionItemKind AI_MouseButton::kind() const
{
    return ActionItemKind::MouseButton;
}

ExecuteResult AI_MouseButton::execute(ActionExecutionContext &context)
{
    auto &executor = App::getInstance().executor;

    if (holdDuration <= 0.0f)
    {
        std::cerr << "[AI_MouseButton] tap button=" << mouseButtonName(button) << " ("
                  << button << ") mods=" << modifiers << '\n';
        executor.modifiersDown(modifiers);
        executor.mouseButton(button, true);
        executor.mouseButton(button, false);
        executor.modifiersUp(modifiers);
        return ExecuteResult::Continue();
    }

    const auto hold = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<float>(holdDuration));
    const auto holdMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(hold).count();

    std::cerr << "[AI_MouseButton] press button=" << mouseButtonName(button)
              << " mods=" << modifiers << " holdMs=" << holdMs
              << " proceed=" << (proceed ? "true" : "false") << '\n';
    executor.modifiersDown(modifiers);
    executor.mouseButton(button, true);

    context.setCancelFlush(makeMouseButtonReleaseAction(button, modifiers));

    std::vector<std::unique_ptr<ActionItem>> delayedItems;
    delayedItems.push_back(std::make_unique<AI_Delay>(static_cast<int>(holdMs)));
    delayedItems.push_back(std::make_unique<AI_MouseButtonRelease>(button, modifiers));
    auto delayed = std::make_unique<Action>(
        std::move(delayedItems), "mouse-hold-release", "", "mouse-hold-release", 0);
    delayed->setCancelable(false);
    // Start now; AI_Delay inside holds for `holdMs`. Do not also wake at
    // now+hold or the release waits 2x holdDuration.
    context.scheduleAction(
        std::move(delayed),
        std::chrono::steady_clock::now(),
        /*removeIfParentCancelled=*/true);

    std::cerr << "[AI_MouseButton] scheduled delayed release + cancel-flush registered\n";

    if (!proceed)
    {
        return ExecuteResult::DelayUntil(std::chrono::steady_clock::now() + hold);
    }

    return ExecuteResult::Continue();
}

AI_MouseButtonRelease::AI_MouseButtonRelease(int button, int modifiers)
    : button{button}
    , modifiers{modifiers}
{
}

std::unique_ptr<ActionItem> AI_MouseButtonRelease::clone() const
{
    return std::make_unique<AI_MouseButtonRelease>(*this);
}

ActionItemKind AI_MouseButtonRelease::kind() const
{
    return ActionItemKind::MouseButtonRelease;
}

ExecuteResult AI_MouseButtonRelease::execute(ActionExecutionContext & /*context*/)
{
    auto &executor = App::getInstance().executor;
    std::cerr << "[AI_MouseButtonRelease] release button=" << mouseButtonName(button) << " ("
              << button << ") mods=" << modifiers << '\n';
    executor.mouseButton(button, false);
    executor.modifiersUp(modifiers);
    return ExecuteResult::Continue();
}

} // namespace Application
