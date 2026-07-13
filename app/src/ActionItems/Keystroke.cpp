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
        // When Platform exposes keyDown/keyUp:
        // App::getInstance().executor.keyDown(bind);
        // App::getInstance().executor.keyUp(bind);
        std::cerr << "[AI_Keystroke] tap keyDown+keyUp key=" << keycode
                  << " mods=" << modifiers << '\n';
        // Stand-in: full tap via existing API.
        App::getInstance().executor.executeKey(bind);
        return ExecuteResult::Continue();
    }

    const auto hold = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<float>(holdDuration));
    const auto holdMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(hold).count();

    // When Platform exposes keyDown:
    // App::getInstance().executor.keyDown(bind);
    std::cerr << "[AI_Keystroke] keyDown key=" << keycode << " mods=" << modifiers
              << " holdMs=" << holdMs << " proceed=" << (proceed ? "true" : "false") << '\n';

    // Immediate KeyUp if this Action is cancelled mid-hold.
    context.setCancelFlush(makeKeyReleaseAction(keycode, modifiers));

    // Safety-net delayed KeyUp (uncancelable; discarded if parent cancel flushes).
    std::vector<std::unique_ptr<ActionItem>> delayedItems;
    delayedItems.push_back(std::make_unique<AI_Delay>(static_cast<int>(holdMs)));
    delayedItems.push_back(std::make_unique<AI_KeyRelease>(keycode, modifiers));
    auto delayed = std::make_unique<Action>(
        std::move(delayedItems), "key-hold-release", "", "key-hold-release", 0);
    delayed->setCancelable(false);
    context.scheduleAction(
        std::move(delayed),
        std::chrono::steady_clock::now() + hold,
        /*removeIfParentCancelled=*/true);

    std::cerr << "[AI_Keystroke] scheduled delayed keyUp + cancel-flush registered\n";

    // Stand-in: when !proceed, also park this Action so following items wait out the hold
    // (real keyDown does not block; sequencing uses DelayUntil until Platform hold is enough).
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

    // When Platform exposes keyUp:
    // App::getInstance().executor.keyUp(bind);
    std::cerr << "[AI_KeyRelease] keyUp key=" << keycode << " mods=" << modifiers << '\n';
    (void)bind;

    return ExecuteResult::Continue();
}

} // namespace Application
