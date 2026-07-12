/**
 * @file ActionItems.cpp
 * @brief ActionItem execute implementations.
 */

#include "App/ActionItems.hpp"

#include "App/App.hpp"

#include <Platform/Inputs.hpp>

#include <chrono>
#include <iostream>

namespace Application
{

std::unique_ptr<ActionItem> ActionItem::clone() const
{
    return std::make_unique<ActionItem>(*this);
}

ActionItemKind ActionItem::kind() const
{
    return ActionItemKind::Base;
}

ExecuteResult ActionItem::execute(ActionExecutionContext & /*context*/)
{
    std::cerr << "ActionItem base execute() called; nothing to do.\n";
    return ExecuteResult::Continue();
}

AI_Script::AI_Script(std::string filepath)
    : filepath{std::move(filepath)}
{
}

std::unique_ptr<ActionItem> AI_Script::clone() const
{
    return std::make_unique<AI_Script>(*this);
}

ActionItemKind AI_Script::kind() const
{
    return ActionItemKind::Script;
}

ExecuteResult AI_Script::execute(ActionExecutionContext & /*context*/)
{
    App::getInstance().executor.executeScript(filepath);
    return ExecuteResult::Continue();
}

AI_LaunchApp::AI_LaunchApp(std::string presetId, std::string customTarget)
    : presetId{std::move(presetId)}
    , customTarget{std::move(customTarget)}
{
}

std::unique_ptr<ActionItem> AI_LaunchApp::clone() const
{
    return std::make_unique<AI_LaunchApp>(*this);
}

ActionItemKind AI_LaunchApp::kind() const
{
    return ActionItemKind::LaunchApp;
}

ExecuteResult AI_LaunchApp::execute(ActionExecutionContext & /*context*/)
{
    if (presetId == "custom")
    {
        if (!customTarget.empty())
        {
            App::getInstance().executor.executeScript(customTarget);
        }
        return ExecuteResult::Continue();
    }

    App::getInstance().executor.executeLaunchPreset(presetId);
    return ExecuteResult::Continue();
}

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

ExecuteResult AI_Keystroke::execute(ActionExecutionContext & /*context*/)
{
    // Ideal hold path once Platform exposes keyDown/keyUp:
    //
    //   platform.keyDown(bind);
    //   if (holdDuration > 0.0f && !proceed) {
    //       auto release = std::make_unique<Action>(/*channel*/ 0);
    //       release->addItem(std::make_unique<AI_KeyRelease>(bind));
    //       context.scheduleAction(
    //           std::move(release),
    //           std::chrono::steady_clock::now()
    //               + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
    //                     std::chrono::duration<float>(holdDuration)));
    //       return ExecuteResult::Continue();
    //   }
    //   platform.keyUp(bind);
    //   return ExecuteResult::Continue();

    Platform::InputBind bind{keycode, modifiers};
    App::getInstance().executor.executeKey(bind);

    // Until keyDown/keyUp exist, a full tap is sent above. If the Action should
    // wait before the next item, delay via the scheduler instead of sleeping.
    if (holdDuration > 0.0f && !proceed)
    {
        const auto hold = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<float>(holdDuration));
        return ExecuteResult::DelayUntil(std::chrono::steady_clock::now() + hold);
    }

    return ExecuteResult::Continue();
}

AI_Delay::AI_Delay(int durationMs)
    : duration{durationMs}
{
}

std::unique_ptr<ActionItem> AI_Delay::clone() const
{
    return std::make_unique<AI_Delay>(*this);
}

ActionItemKind AI_Delay::kind() const
{
    return ActionItemKind::Delay;
}

ExecuteResult AI_Delay::execute(ActionExecutionContext & /*context*/)
{
    return ExecuteResult::DelayUntil(
        std::chrono::steady_clock::now() + std::chrono::milliseconds(duration));
}

AI_Menu::AI_Menu(std::string menuId)
    : menuId{std::move(menuId)}
{
}

std::unique_ptr<ActionItem> AI_Menu::clone() const
{
    return std::make_unique<AI_Menu>(*this);
}

ActionItemKind AI_Menu::kind() const
{
    return ActionItemKind::Menu;
}

ExecuteResult AI_Menu::execute(ActionExecutionContext & /*context*/)
{
    App &app = App::getInstance();
    Menu *targetMenu = app.findMenuById(menuId);
    if (targetMenu != nullptr)
    {
        app.showGui(targetMenu);
    }
    else
    {
        std::cerr << "Failed to find menu with id: '" << menuId << "'\n";
    }
    return ExecuteResult::Continue();
}

std::unique_ptr<ActionItem> AI_Close::clone() const
{
    return std::make_unique<AI_Close>(*this);
}

ActionItemKind AI_Close::kind() const
{
    return ActionItemKind::Close;
}

ExecuteResult AI_Close::execute(ActionExecutionContext & /*context*/)
{
    App::getInstance().hideGui();
    return ExecuteResult::Continue();
}

std::unique_ptr<ActionItem> AI_Socket::clone() const
{
    return std::make_unique<AI_Socket>(*this);
}

ActionItemKind AI_Socket::kind() const
{
    return ActionItemKind::Socket;
}

ExecuteResult AI_Socket::execute(ActionExecutionContext & /*context*/)
{
    // Predicted once Platform gains a socket/send API:
    //
    //   App::getInstance().executor.sendSocket(outputDst, socketMsg);
    //   return ExecuteResult::Continue();

    std::cerr << "AI_Socket not implemented yet (msg='" << socketMsg
              << "', dst='" << outputDst << "').\n";
    return ExecuteResult::Continue();
}

std::unique_ptr<ActionItem> AI_nthRecent::clone() const
{
    return std::make_unique<AI_nthRecent>(*this);
}

ActionItemKind AI_nthRecent::kind() const
{
    return ActionItemKind::NthRecent;
}

ExecuteResult AI_nthRecent::execute(ActionExecutionContext & /*context*/)
{
    // Predicted once action-history tracking exists:
    //
    //   Action* recent = App::getInstance().nthRecentAction(n);
    //   if (recent != nullptr) {
    //       context.scheduleAction(
    //           std::make_unique<Action>(*recent),  // or a dedicated clone helper
    //           std::chrono::steady_clock::now());
    //   }
    //   return ExecuteResult::Continue();

    std::cerr << "AI_nthRecent not implemented yet (n=" << n << ").\n";
    return ExecuteResult::Continue();
}

std::unique_ptr<ActionItem> AI_nthFrequent::clone() const
{
    return std::make_unique<AI_nthFrequent>(*this);
}

ActionItemKind AI_nthFrequent::kind() const
{
    return ActionItemKind::NthFrequent;
}

ExecuteResult AI_nthFrequent::execute(ActionExecutionContext & /*context*/)
{
    // Predicted once usage-frequency tracking exists:
    //
    //   Action* frequent = App::getInstance().nthFrequentAction(n);
    //   if (frequent != nullptr) {
    //       context.scheduleAction(
    //           std::make_unique<Action>(*frequent),
    //           std::chrono::steady_clock::now());
    //   }
    //   return ExecuteResult::Continue();

    std::cerr << "AI_nthFrequent not implemented yet (n=" << n << ").\n";
    return ExecuteResult::Continue();
}

} // namespace Application
