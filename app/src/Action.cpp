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
#include <typeinfo>
#include <memory>
#include <vector>

using namespace Application;

Action::Action(std::vector<std::unique_ptr<ActionItem>> _sequence, std::string _name, std::string _iconFilepath) : sequence(std::move(_sequence)), name(_name), iconFilepath(_iconFilepath)
{
}

Action::Action(const Action &other) : name(other.name), iconFilepath(other.iconFilepath)
{
    sequence.reserve(other.sequence.size());
    for (const auto &item : other.sequence)
    {
        if (item)
        {
            sequence.push_back(item->clone());
        }
    }
}

Action &Action::operator=(const Action &other)
{
    if (this == &other)
    {
        return *this;
    }

    name = other.name;
    iconFilepath = other.iconFilepath;
    sequence.clear();
    sequence.reserve(other.sequence.size());
    for (const auto &item : other.sequence)
    {
        if (item)
        {
            sequence.push_back(item->clone());
        }
    }

    return *this;
}

Action::Action(Action &&other) noexcept = default;

Action &Action::operator=(Action &&other) noexcept = default;

Action::~Action() = default;

void Action::addItem(int ind, std::unique_ptr<ActionItem> ai)
{
    if (!ai)
    {
        return;
    }

    if (ind < 0 || ind >= static_cast<int>(sequence.size()))
    {
        sequence.push_back(std::move(ai));
        return;
    }

    sequence.insert(sequence.begin() + ind, std::move(ai));
}

void Action::removeItem(int ind)
{
    if (ind >= 0 && ind < static_cast<int>(sequence.size()))
    {
        sequence.erase(sequence.begin() + ind);
    }
}

int Action::len()
{
    return this->sequence.size();
}

void Action::execute()
{
    for (const auto &item : sequence)
    {
        if (item)
        {
            item->execute();
        }
    }
}

std::string Action::getName() const
{
    return name;
}

bool Action::isScriptAction() const
{
    if (sequence.size() != 1 || !sequence.front())
    {
        return false;
    }

    return dynamic_cast<AI_Script *>(sequence.front().get()) != nullptr;
}

std::string Action::getScriptPath() const
{
    if (!isScriptAction())
    {
        return "";
    }

    return static_cast<AI_Script *>(sequence.front().get())->filepath;
}

void Action::setName(const std::string &newName)
{
    name = newName;
}

void Action::setScriptAction(const std::string &path)
{
    sequence.clear();
    sequence.push_back(std::make_unique<AI_Script>(path));
}
