/**
 * @file phase9_actionitems_smoke.cpp
 * @brief Disposable smoke tests for built-in ActionItems (Phase 9).
 */

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/ActionItems.hpp"
#include "App/ExecuteResult.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

using namespace Application;

namespace
{

bool testDelay()
{
    AI_Delay delay{50};
    ActionExecutionContext ctx{std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "t", "", "t", 0)};
    const ExecuteResult result = delay.execute(ctx);
    if (result.type() != ExecuteResult::Type::DelayUntil)
    {
        std::cerr << "delay: expected DelayUntil\n";
        return false;
    }
    return true;
}

bool testKeystrokeHoldRegistersCleanup()
{
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<AI_Keystroke>('A', 0, 0.05f, true));
    auto action = std::make_unique<Action>(std::move(items), "hold", "", "hold", 1);
    ActionExecutionContext ctx{std::move(action)};

    ActionItem *item = ctx.currentItem();
    if (item == nullptr)
    {
        std::cerr << "hold: no item\n";
        return false;
    }

    const ExecuteResult result = item->execute(ctx);
    if (result.type() != ExecuteResult::Type::Continue)
    {
        std::cerr << "hold: expected Continue when proceed=true\n";
        return false;
    }
    if (!ctx.hasPendingSchedulerRequests())
    {
        std::cerr << "hold: expected cancel-flush / scheduleAction requests\n";
        return false;
    }

    auto flushes = ctx.takeCancelFlushes();
    auto scheduled = ctx.takeScheduledActions();
    if (flushes.empty() || scheduled.empty())
    {
        std::cerr << "hold: missing flush or scheduled release\n";
        return false;
    }
    if (flushes.front()->cancelable() || scheduled.front().action->cancelable())
    {
        std::cerr << "hold: cleanup Actions should be uncancelable\n";
        return false;
    }
    if (!scheduled.front().removeIfParentCancelled)
    {
        std::cerr << "hold: expected removeIfParentCancelled\n";
        return false;
    }
    return true;
}

bool testKeystrokeHoldBlocksWhenNotProceed()
{
    AI_Keystroke key{'B', 0, 0.05f, false};
    ActionExecutionContext ctx{std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "t", "", "t", 0)};
    const ExecuteResult result = key.execute(ctx);
    if (result.type() != ExecuteResult::Type::DelayUntil)
    {
        std::cerr << "hold_block: expected DelayUntil when proceed=false\n";
        return false;
    }
    if (!ctx.hasPendingSchedulerRequests())
    {
        std::cerr << "hold_block: expected cleanup registration\n";
        return false;
    }
    return true;
}

bool testMouseMoveAndButton()
{
    AI_MouseMove move{10, 20};
    AI_MouseButton button{1, false};
    ActionExecutionContext ctx{std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "t", "", "t", 0)};

    // MouseMove calls Platform setMousePos — skip execute in headless smoke.
    if (move.kind() != ActionItemKind::MouseMove || move.clone()->kind() != ActionItemKind::MouseMove)
    {
        std::cerr << "mouse_move: wrong kind/clone\n";
        return false;
    }
    if (move.x != 10 || move.y != 20)
    {
        std::cerr << "mouse_move: coords not stored\n";
        return false;
    }

    if (button.execute(ctx).type() != ExecuteResult::Type::Continue)
    {
        std::cerr << "mouse_button: expected Continue\n";
        return false;
    }
    if (button.kind() != ActionItemKind::MouseButton)
    {
        std::cerr << "mouse_button: wrong kind\n";
        return false;
    }
    return true;
}

bool testKeyRelease()
{
    AI_KeyRelease release{'Z', 0};
    ActionExecutionContext ctx{std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "t", "", "t", 0)};
    if (release.execute(ctx).type() != ExecuteResult::Type::Continue)
    {
        std::cerr << "key_release: expected Continue\n";
        return false;
    }
    return true;
}

} // namespace

int main()
{
    using TestFn = bool (*)();
    const std::pair<const char *, TestFn> tests[] = {
        {"delay", testDelay},
        {"keystroke_hold_cleanup", testKeystrokeHoldRegistersCleanup},
        {"keystroke_hold_blocks", testKeystrokeHoldBlocksWhenNotProceed},
        {"mouse_move_button", testMouseMoveAndButton},
        {"key_release", testKeyRelease},
    };

    int failed = 0;
    for (const auto &[name, fn] : tests)
    {
        std::cout << "[RUN ] " << name << '\n';
        if (!fn())
        {
            std::cout << "[FAIL] " << name << '\n';
            ++failed;
        }
        else
        {
            std::cout << "[PASS] " << name << '\n';
        }
    }

    if (failed != 0)
    {
        std::cerr << failed << " phase9 actionitems smoke test(s) failed\n";
        return 1;
    }

    std::cout << "Phase 9 ActionItems smoke tests passed\n";
    return 0;
}
