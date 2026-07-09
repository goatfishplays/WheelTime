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

void ActionItem::execute()
{
    std::cerr << "Action Item base class exectued";
}

AI_Script::AI_Script(std::string _filepath) : filepath(_filepath) {}

std::unique_ptr<ActionItem> AI_Script::clone() const
{
    return std::make_unique<AI_Script>(*this);
}

void AI_Script::execute()
{
    App::App::getInstance().executor.executeScript(filepath);
}

std::unique_ptr<ActionItem> AI_Keystroke::clone() const
{
    return std::make_unique<AI_Keystroke>(*this);
}

void AI_Keystroke::execute() {}

std::unique_ptr<ActionItem> AI_Delay::clone() const
{
    return std::make_unique<AI_Delay>(*this);
}

void AI_Delay::execute()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));
}

std::unique_ptr<ActionItem> AI_Menu::clone() const
{
    return std::make_unique<AI_Menu>(*this);
}

void AI_Menu::execute()
{
    App &app = App::App::getInstance();
    for (int i = 0; i < app.loadedMenus.size(); i++)
    {
        if (app.loadedMenus[i]->getName() == menuName)
        {
            app.showGui(app.loadedMenus[i]);
            return;
        }
    }
    std::cerr << "Failed to find menu with name: '" << menuName << "'\n";
}

void AI_Close::execute() { App::App::getInstance().hideGui(); }

std::unique_ptr<ActionItem> AI_Close::clone() const
{
    return std::make_unique<AI_Close>(*this);
}

std::unique_ptr<ActionItem> AI_Socket::clone() const
{
    return std::make_unique<AI_Socket>(*this);
}

void AI_Socket::execute() {}

std::unique_ptr<ActionItem> AI_nthRecent::clone() const
{
    return std::make_unique<AI_nthRecent>(*this);
}

void AI_nthRecent::execute() {}

std::unique_ptr<ActionItem> AI_nthFrequent::clone() const
{
    return std::make_unique<AI_nthFrequent>(*this);
}

void AI_nthFrequent::execute() {}
