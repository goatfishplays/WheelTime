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

void ActionItem::execute()
{
    std::cerr << "Action Item base class exectued";
}

void AI_Script::execute()
{
    App::App::getInstance().executor.executeScript(filepath);
}

void AI_Keystroke::execute() {}

void AI_Delay::execute()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));
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

void AI_Socket::execute() {}

void AI_nthRecent::execute() {}

void AI_nthFrequent::execute() {}
