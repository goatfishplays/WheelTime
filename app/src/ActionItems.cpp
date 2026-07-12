/**
 * @file ActionItems.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "App/ActionItems.hpp"
#include "App/App.hpp"
#include <string>
#include <iostream>
#include <chrono>
#include <thread>

using namespace Application;

ActionItem::ActionItem() {}

ActionItem::~ActionItem() {}

std::unique_ptr<ActionItem> ActionItem::clone() const
{
    return std::make_unique<ActionItem>(*this);
}

ActionItemKind ActionItem::kind() const
{
    return ActionItemKind::Base;
}

ExecuteResult ActionItem::execute(ActionExecutionContext &context)
{
    std::cerr << "Action Item base class exectued";
}

AI_Script::AI_Script(std::string _filepath) : filepath(_filepath) {}

std::unique_ptr<ActionItem> AI_Script::clone() const
{
    return std::make_unique<AI_Script>(*this);
}

ActionItemKind AI_Script::kind() const
{
    return ActionItemKind::Script;
}

ExecuteResult AI_Script::execute(ActionExecutionContext &context)
{
    App::App::getInstance().executor.executeScript(filepath);
}

AI_LaunchApp::AI_LaunchApp(std::string _presetId, std::string _customTarget)
    : presetId(std::move(_presetId)), customTarget(std::move(_customTarget))
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

ExecuteResult AI_LaunchApp::execute(ActionExecutionContext &context)
{
    if (presetId == "custom")
    {
        if (!customTarget.empty())
        {
            App::App::getInstance().executor.executeScript(customTarget);
        }
        return;
    }

    App::App::getInstance().executor.executeLaunchPreset(presetId);
}

AI_Keystroke::AI_Keystroke(int _keycode, int _modifiers, float _holdDuration, bool _proceed)
    : keycode(_keycode), modifiers(_modifiers), holdDuration(_holdDuration), proceed(_proceed)
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

    // m_platform.keyDown(m_key); // TODO: final should be of this form

    // auto release = std::make_unique<Action>(0);

    // release->addItem(
    //     std::make_unique<KeyReleaseItem>(m_key));

    // context.scheduleAction(
    //     std::move(release),
    //     std::chrono::steady_clock::now() + m_holdTime);

    // return ExecuteResult::Continue();

    Platform::InputBind bind{keycode, modifiers};
    App::App::getInstance().executor.executeKey(bind);

    if (holdDuration > 0.0f && !proceed)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(holdDuration * 1000.0f)));
    }
}

AI_Delay::AI_Delay(int _duration) : duration(_duration) {}

std::unique_ptr<ActionItem> AI_Delay::clone() const
{
    return std::make_unique<AI_Delay>(*this);
}

ActionItemKind AI_Delay::kind() const
{
    return ActionItemKind::Delay;
}

ExecuteResult AI_Delay::execute(ActionExecutionContext &context)
{
    return ExecuteResult::DelayUntil(
        std::chrono::steady_clock::now() + m_duration);
}

AI_Menu::AI_Menu(std::string _menuId) : menuId(std::move(_menuId)) {}

std::unique_ptr<ActionItem> AI_Menu::clone() const
{
    return std::make_unique<AI_Menu>(*this);
}

ActionItemKind AI_Menu::kind() const
{
    return ActionItemKind::Menu;
}

ExecuteResult AI_Menu::execute(ActionExecutionContext &context)
{
    App &app = App::App::getInstance();
    Menu *targetMenu = app.findMenuById(menuId);
    if (targetMenu != nullptr)
    {
        app.showGui(targetMenu);
        return;
    }
    std::cerr << "Failed to find menu with id: '" << menuId << "'\n";
}

ExecuteResult AI_Close::execute(ActionExecutionContext &context) { App::App::getInstance().hideGui(); }

std::unique_ptr<ActionItem> AI_Close::clone() const
{
    return std::make_unique<AI_Close>(*this);
}

ActionItemKind AI_Close::kind() const
{
    return ActionItemKind::Close;
}

std::unique_ptr<ActionItem> AI_Socket::clone() const
{
    return std::make_unique<AI_Socket>(*this);
}

ActionItemKind AI_Socket::kind() const
{
    return ActionItemKind::Socket;
}

ExecuteResult AI_Socket::execute(ActionExecutionContext &context) {}

std::unique_ptr<ActionItem> AI_nthRecent::clone() const
{
    return std::make_unique<AI_nthRecent>(*this);
}

ActionItemKind AI_nthRecent::kind() const
{
    return ActionItemKind::NthRecent;
}

ExecuteResult AI_nthRecent::execute(ActionExecutionContext &context) {}

std::unique_ptr<ActionItem> AI_nthFrequent::clone() const
{
    return std::make_unique<AI_nthFrequent>(*this);
}

ActionItemKind AI_nthFrequent::kind() const
{
    return ActionItemKind::NthFrequent;
}

ExecuteResult AI_nthFrequent::execute(ActionExecutionContext &context) {}
