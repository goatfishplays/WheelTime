/**
 * @file Menu.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "App/Menu.hpp"
#include <Platform/Inputs.hpp>
#include <vector>
#include <string>

using namespace Application;

Menu::Menu(Platform::InputBind *_inputBind,
           bool _executeOnRelease,
           bool _exitOnAction,
           std::string _name,
           std::vector<std::string> _actionIds,
           std::string _id)
    : inputBind(_inputBind), executeOnRelease(_executeOnRelease), exitOnAction(_exitOnAction), name(std::move(_name)), id(std::move(_id)), actionIds(std::move(_actionIds))
{
}

Menu::~Menu() {};

void Menu::addActionId(int ind, const std::string &actionId)
{
    if (ind < 0 || ind >= static_cast<int>(actionIds.size()))
    {
        actionIds.push_back(actionId);
        return;
    }

    actionIds.insert(actionIds.begin() + ind, actionId);
}

void Menu::remAction(int ind)
{
    if (ind >= 0 && ind < static_cast<int>(actionIds.size()))
    {
        actionIds.erase(actionIds.begin() + ind);
    }
}

int Menu::numActions() const { return static_cast<int>(this->actionIds.size()); }

std::string Menu::getId() const
{
    return id;
}

std::string Menu::getName() const
{
    return name;
}

void Menu::setId(const std::string &newId)
{
    id = newId;
}

void Menu::setName(const std::string &newName)
{
    name = newName;
}

const std::vector<std::string> &Menu::getActionIds() const
{
    return actionIds;
}

std::string Menu::getActionId(int index) const
{
    if (index < 0 || index >= static_cast<int>(actionIds.size()))
    {
        return "";
    }

    return actionIds[index];
}

void Menu::setActionId(int index, const std::string &actionId)
{
    if (index >= 0 && index < static_cast<int>(actionIds.size()))
    {
        actionIds[index] = actionId;
    }
}

void Menu::moveActionId(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || toIndex < 0 || fromIndex >= static_cast<int>(actionIds.size()) || toIndex >= static_cast<int>(actionIds.size()) || fromIndex == toIndex)
    {
        return;
    }

    std::string actionId = actionIds[fromIndex];
    actionIds.erase(actionIds.begin() + fromIndex);
    actionIds.insert(actionIds.begin() + toIndex, actionId);
}
