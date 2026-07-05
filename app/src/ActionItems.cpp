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
#include <string>
#include <iostream>

using namespace Application;

ActionItem::ActionItem() {}

ActionItem::~ActionItem() {}

void ActionItem::execute()
{
    std::cerr << "Action Item base class exectued";
}

void AI_Script::execute()
{
}

void AI_Keystroke::execute() {}

void AI_Delay::execute() {}

void AI_Menu::execute() {}

void AI_Close::execute() {}

void AI_Socket::execute() {}

void AI_nthRecent::execute() {}

void AI_nthFrequent::execute() {}
