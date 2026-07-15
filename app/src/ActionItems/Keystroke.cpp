/**
 * @file Keystroke.cpp
 * @brief AI_Keystroke / AI_KeyRelease definitions.
 */

#include "App/ActionItems/Keystroke.hpp"

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/ActionItems/Delay.hpp"
#include "App/App.hpp"

#include <Platform/Inputs.hpp>

#include <chrono>
#include <iostream>
#include <vector>

namespace Application
{
namespace
{

std::unique_ptr<Action> makeKeyReleaseAction(int keycode, int modifiers)
{
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<AI_KeyRelease>(keycode, modifiers));
    auto action = std::make_unique<Action>(std::move(items), "key-release", "", "key-release", 0);
    action->setCancelable(false);
    return action;
}

} // namespace

AI_Keystroke::AI_Keystroke(int keycode, int modifiers, float holdDuration, bool proceed)
    : keycode{keycode}
    , modifiers{modifiers}
    , holdDuration{holdDuration}
    , proceed{proceed}
{
}

std::unique_ptr<ActionItem> AI_Keystroke::clone() const
{
    return std::make_unique<AI_Keystroke>(*this);
}

ActionItemKind AI_Keystroke::kind() const
{
    return ActionItemKind::Keystroke;
}

ExecuteResult AI_Keystroke::execute(ActionExecutionContext &context)
{
    Platform::InputBind bind{keycode, modifiers};

    if (holdDuration <= 0.0f)
    {
        std::cerr << "[AI_Keystroke] tap key=" << keycode << " mods=" << modifiers << '\n';
        App::getInstance().executor.executeKey(bind);
        return ExecuteResult::Continue();
    }

    const auto hold = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<float>(holdDuration));
    const auto holdMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(hold).count();

    std::cerr << "[AI_Keystroke] keyDown key=" << keycode << " mods=" << modifiers
              << " holdMs=" << holdMs << " proceed=" << (proceed ? "true" : "false") << '\n';
    App::getInstance().executor.keyDown(bind);

    // Immediate KeyUp if this Action is cancelled mid-hold.
    context.setCancelFlush(makeKeyReleaseAction(keycode, modifiers));

    // Safety-net delayed KeyUp (uncancelable; discarded if parent cancel flushes).
    std::vector<std::unique_ptr<ActionItem>> delayedItems;
    delayedItems.push_back(std::make_unique<AI_Delay>(static_cast<int>(holdMs)));
    delayedItems.push_back(std::make_unique<AI_KeyRelease>(keycode, modifiers));
    auto delayed = std::make_unique<Action>(
        std::move(delayedItems), "key-hold-release", "", "key-hold-release", 0);
    delayed->setCancelable(false);
    // Start now; AI_Delay inside holds for `holdMs`. Do not also wake at
    // now+hold or the release waits 2x holdDuration.
    context.scheduleAction(
        std::move(delayed),
        std::chrono::steady_clock::now(),
        /*removeIfParentCancelled=*/true);

    std::cerr << "[AI_Keystroke] scheduled delayed keyUp + cancel-flush registered\n";

    if (!proceed)
    {
        return ExecuteResult::DelayUntil(std::chrono::steady_clock::now() + hold);
    }

    return ExecuteResult::Continue();
}

AI_KeyRelease::AI_KeyRelease(int keycode, int modifiers)
    : keycode{keycode}
    , modifiers{modifiers}
{
}

std::unique_ptr<ActionItem> AI_KeyRelease::clone() const
{
    return std::make_unique<AI_KeyRelease>(*this);
}

ActionItemKind AI_KeyRelease::kind() const
{
    return ActionItemKind::KeyRelease;
}

ExecuteResult AI_KeyRelease::execute(ActionExecutionContext & /*context*/)
{
    Platform::InputBind bind{keycode, modifiers};
    std::cerr << "[AI_KeyRelease] keyUp key=" << keycode << " mods=" << modifiers << '\n';
    App::getInstance().executor.keyUp(bind);
    return ExecuteResult::Continue();
}

} // namespace Application
