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

Action::Action(std::vector<std::unique_ptr<ActionItem>> _sequence, std::string _name, std::string _iconFilepath, std::string _id, uint32_t _channel)
    : sequence(std::move(_sequence)), name(std::move(_name)), iconFilepath(std::move(_iconFilepath)), id(std::move(_id)), m_channel(_channel)
{
}

Action::Action(const Action &other) : name(other.name), iconFilepath(other.iconFilepath), id(other.id)
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
    id = other.id;
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

int Action::len() const
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

std::string Action::getId() const
{
    return id;
}

std::string Action::getName() const
{
    return name;
}

const std::vector<std::unique_ptr<ActionItem>> &Action::getItems() const
{
    return sequence;
}

ActionItem *Action::getItem(int index)
{
    if (index < 0 || index >= static_cast<int>(sequence.size()))
    {
        return nullptr;
    }

    return sequence[index].get();
}

const ActionItem *Action::getItem(int index) const
{
    if (index < 0 || index >= static_cast<int>(sequence.size()))
    {
        return nullptr;
    }

    return sequence[index].get();
}

void Action::setName(const std::string &newName)
{
    name = newName;
}

void Action::setId(const std::string &newId)
{
    id = newId;
}

void Action::setItems(std::vector<std::unique_ptr<ActionItem>> newItems)
{
    sequence = std::move(newItems);
}

void Action::moveItem(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || toIndex < 0 || fromIndex >= static_cast<int>(sequence.size()) || toIndex >= static_cast<int>(sequence.size()) || fromIndex == toIndex)
    {
        return;
    }

    std::unique_ptr<ActionItem> item = std::move(sequence[fromIndex]);
    sequence.erase(sequence.begin() + fromIndex);
    sequence.insert(sequence.begin() + toIndex, std::move(item));
}

uint32_t Action::channel() const
{
    return m_channel;
}

const std::vector<std::unique_ptr<ActionItem>> &Action::items() const
{
    return sequence;
}