/**
 * @file Action.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "App/Action.hpp"
#include "App/ActionItems.hpp"
#include <vector>

using namespace Application;

Action::Action(std::vector<ActionItem> _sequence) : sequence(_sequence)
{
}

Action::~Action() {}

void Action::addItem(int ind, ActionItem ai) {}

void Action::removeItem(int ind) {}

int Action::len() {}

void Action::execute() {}