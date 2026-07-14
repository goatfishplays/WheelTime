/**
 * @file Action.cpp
 * @brief Action definitions.
 */

#include "App/Action.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace Application
{

Action::Action(std::vector<std::unique_ptr<ActionItem>> sequence,
               std::string name,
               std::string iconFilepath,
               std::string id,
               uint32_t channel)
    : m_sequence(std::move(sequence))
    , m_name(std::move(name))
    , m_iconFilepath(std::move(iconFilepath))
    , m_id(std::move(id))
    , m_channel(channel)
    , m_cancelable(true)
{
}

Action::Action(const Action &other)
    : m_name(other.m_name)
    , m_iconFilepath(other.m_iconFilepath)
    , m_id(other.m_id)
    , m_channel(other.m_channel)
    , m_cancelable(other.m_cancelable)
{
    m_sequence.reserve(other.m_sequence.size());
    for (const auto &item : other.m_sequence)
    {
        if (item)
        {
            m_sequence.push_back(item->clone());
        }
    }
}

Action &Action::operator=(const Action &other)
{
    if (this == &other)
    {
        return *this;
    }

    m_name = other.m_name;
    m_iconFilepath = other.m_iconFilepath;
    m_id = other.m_id;
    m_channel = other.m_channel;
    m_cancelable = other.m_cancelable;
    m_sequence.clear();
    m_sequence.reserve(other.m_sequence.size());
    for (const auto &item : other.m_sequence)
    {
        if (item)
        {
            m_sequence.push_back(item->clone());
        }
    }

    return *this;
}

Action::Action(Action &&other) noexcept = default;

Action &Action::operator=(Action &&other) noexcept = default;

Action::~Action() = default;

void Action::addItem(int index, std::unique_ptr<ActionItem> ai)
{
    if (!ai)
    {
        return;
    }

    if (index < 0 || index >= static_cast<int>(m_sequence.size()))
    {
        m_sequence.push_back(std::move(ai));
        return;
    }

    m_sequence.insert(m_sequence.begin() + index, std::move(ai));
}

void Action::removeItem(int index)
{
    if (index >= 0 && index < static_cast<int>(m_sequence.size()))
    {
        m_sequence.erase(m_sequence.begin() + index);
    }
}

int Action::itemCount() const
{
    return static_cast<int>(m_sequence.size());
}

std::string Action::getId() const
{
    return m_id;
}

std::string Action::getName() const
{
    return m_name;
}

std::string Action::getIconFilepath() const
{
    return m_iconFilepath;
}

const std::vector<std::unique_ptr<ActionItem>> &Action::getItems() const
{
    return m_sequence;
}

ActionItem *Action::getItem(int index)
{
    if (index < 0 || index >= static_cast<int>(m_sequence.size()))
    {
        return nullptr;
    }

    return m_sequence[index].get();
}

const ActionItem *Action::getItem(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_sequence.size()))
    {
        return nullptr;
    }

    return m_sequence[index].get();
}

void Action::setName(const std::string &newName)
{
    m_name = newName;
}

void Action::setId(const std::string &newId)
{
    m_id = newId;
}

void Action::setIconFilepath(const std::string &newIconFilepath)
{
    m_iconFilepath = newIconFilepath;
}

void Action::setItems(std::vector<std::unique_ptr<ActionItem>> newItems)
{
    m_sequence = std::move(newItems);
}

void Action::moveItem(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || toIndex < 0 || fromIndex >= static_cast<int>(m_sequence.size())
        || toIndex >= static_cast<int>(m_sequence.size()) || fromIndex == toIndex)
    {
        return;
    }

    std::unique_ptr<ActionItem> item = std::move(m_sequence[fromIndex]);
    m_sequence.erase(m_sequence.begin() + fromIndex);
    m_sequence.insert(m_sequence.begin() + toIndex, std::move(item));
}

uint32_t Action::channel() const
{
    return m_channel;
}

void Action::setChannel(uint32_t channel) noexcept
{
    m_channel = channel;
}

bool Action::cancelable() const noexcept
{
    return m_cancelable;
}

void Action::setCancelable(bool cancelable) noexcept
{
    m_cancelable = cancelable;
}

} // namespace Application
