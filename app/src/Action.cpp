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

Action::Action(std::vector<ActionItem> _sequence, std::string _name, std::string _iconFilepath) : sequence(_sequence), name(_name), iconFilepath(_iconFilepath)
{
}

Action::~Action() {}

void Action::addItem(int ind, ActionItem ai) {}

void Action::removeItem(int ind) {}

int Action::len()
{
    return this->sequence.size();
}

void Action::execute() {}

std::string Action::getName() const
{
    return name;
}