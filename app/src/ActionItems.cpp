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

namespace
{
    std::string resolveLaunchPresetTarget(const std::string &presetId, const std::string &customTarget)
    {
        if (presetId == "browser")
            return "https://www.google.com";
        if (presetId == "explorer")
            return "explorer.exe";
        if (presetId == "calculator")
            return "calc.exe";
        if (presetId == "notepad")
            return "notepad.exe";
        if (presetId == "paint")
            return "mspaint.exe";
        if (presetId == "taskmgr")
            return "taskmgr.exe";
        return customTarget;
    }
}

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

void ActionItem::execute()
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

void AI_Script::execute()
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

void AI_LaunchApp::execute()
{
    const std::string target = resolveLaunchPresetTarget(presetId, customTarget);
    if (!target.empty())
    {
        App::App::getInstance().executor.executeScript(target);
    }
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

void AI_Keystroke::execute()
{
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

void AI_Delay::execute()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));
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

void AI_Menu::execute()
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

void AI_Close::execute() { App::App::getInstance().hideGui(); }

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

void AI_Socket::execute() {}

std::unique_ptr<ActionItem> AI_nthRecent::clone() const
{
    return std::make_unique<AI_nthRecent>(*this);
}

ActionItemKind AI_nthRecent::kind() const
{
    return ActionItemKind::NthRecent;
}

void AI_nthRecent::execute() {}

std::unique_ptr<ActionItem> AI_nthFrequent::clone() const
{
    return std::make_unique<AI_nthFrequent>(*this);
}

ActionItemKind AI_nthFrequent::kind() const
{
    return ActionItemKind::NthFrequent;
}

void AI_nthFrequent::execute() {}
