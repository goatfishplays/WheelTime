/**
 * @file MouseScroll.cpp
 * @brief AI_MouseScroll / AI_MouseScrollRelease / AI_MouseScrollTick definitions.
 */

#include "App/ActionItems/MouseScroll.hpp"

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/ActionItems/Delay.hpp"
#include "App/App.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

namespace Application
{
namespace
{

constexpr float kMinIntervalSec = 0.001f;

std::unique_ptr<Action> makeMouseScrollReleaseAction(int modifiers)
{
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<AI_MouseScrollRelease>(modifiers));
    auto action =
        std::make_unique<Action>(std::move(items), "mouse-scroll-release", "", "mouse-scroll-release", 0);
    action->setCancelable(false);
    return action;
}

} // namespace

AI_MouseScroll::AI_MouseScroll(int dx,
                               int dy,
                               float holdDurationSec,
                               bool proceed,
                               int modifiers,
                               float intervalSec)
    : dx{dx}
    , dy{dy}
    , modifiers{modifiers}
    , holdDurationSec{holdDurationSec}
    , intervalSec{intervalSec}
    , proceed{proceed}
{
}

std::unique_ptr<ActionItem> AI_MouseScroll::clone() const
{
    return std::make_unique<AI_MouseScroll>(*this);
}

ActionItemKind AI_MouseScroll::kind() const
{
    return ActionItemKind::MouseScroll;
}

ExecuteResult AI_MouseScroll::execute(ActionExecutionContext &context)
{
    auto &executor = App::instance().executor();

    if (holdDurationSec <= 0.0f)
    {
        std::cerr << "[AI_MouseScroll] tap dx=" << dx << " dy=" << dy << " mods=" << modifiers
                  << '\n';
        executor.modifiersDown(modifiers);
        executor.mouseScroll(dx, dy);
        executor.modifiersUp(modifiers);
        return ExecuteResult::Continue();
    }

    const float interval = std::max(intervalSec, kMinIntervalSec);
    const auto hold = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<float>(holdDurationSec));
    const auto holdMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(hold).count();
    const auto intervalMs = std::max(
        1,
        static_cast<int>(std::lround(static_cast<double>(interval) * 1000.0)));

    // First tick immediate; then one tick per interval for the rest of the hold.
    int tickCount = static_cast<int>(holdMs / intervalMs) + 1;
    if (tickCount < 1)
    {
        tickCount = 1;
    }

    std::cerr << "[AI_MouseScroll] hold dx=" << dx << " dy=" << dy << " mods=" << modifiers
              << " holdMs=" << holdMs << " intervalMs=" << intervalMs << " ticks=" << tickCount
              << " proceed=" << (proceed ? "true" : "false") << '\n';

    executor.modifiersDown(modifiers);
    context.setCancelFlush(makeMouseScrollReleaseAction(modifiers));

    std::vector<std::unique_ptr<ActionItem>> delayedItems;
    for (int i = 0; i < tickCount; ++i)
    {
        if (i > 0)
        {
            delayedItems.push_back(std::make_unique<AI_Delay>(intervalMs));
        }
        delayedItems.push_back(std::make_unique<AI_MouseScrollTick>(dx, dy));
    }
    delayedItems.push_back(std::make_unique<AI_MouseScrollRelease>(modifiers));

    auto delayed = std::make_unique<Action>(
        std::move(delayedItems), "mouse-scroll-hold", "", "mouse-scroll-hold", 0);
    delayed->setCancelable(false);
    context.scheduleAction(
        std::move(delayed),
        std::chrono::steady_clock::now(),
        /*removeIfParentCancelled=*/true);

    std::cerr << "[AI_MouseScroll] scheduled tick chain + cancel-flush registered\n";

    if (!proceed)
    {
        return ExecuteResult::DelayUntil(std::chrono::steady_clock::now() + hold);
    }

    return ExecuteResult::Continue();
}

AI_MouseScrollRelease::AI_MouseScrollRelease(int modifiers)
    : modifiers{modifiers}
{
}

std::unique_ptr<ActionItem> AI_MouseScrollRelease::clone() const
{
    return std::make_unique<AI_MouseScrollRelease>(*this);
}

ActionItemKind AI_MouseScrollRelease::kind() const
{
    return ActionItemKind::MouseScrollRelease;
}

ExecuteResult AI_MouseScrollRelease::execute(ActionExecutionContext & /*context*/)
{
    std::cerr << "[AI_MouseScrollRelease] mods=" << modifiers << '\n';
    App::instance().executor().modifiersUp(modifiers);
    return ExecuteResult::Continue();
}

AI_MouseScrollTick::AI_MouseScrollTick(int dx, int dy)
    : dx{dx}
    , dy{dy}
{
}

std::unique_ptr<ActionItem> AI_MouseScrollTick::clone() const
{
    return std::make_unique<AI_MouseScrollTick>(*this);
}

ActionItemKind AI_MouseScrollTick::kind() const
{
    return ActionItemKind::MouseScrollTick;
}

ExecuteResult AI_MouseScrollTick::execute(ActionExecutionContext & /*context*/)
{
    std::cerr << "[AI_MouseScrollTick] dx=" << dx << " dy=" << dy << '\n';
    App::instance().executor().mouseScroll(dx, dy);
    return ExecuteResult::Continue();
}

} // namespace Application
