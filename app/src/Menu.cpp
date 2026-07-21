/**
 * @file Menu.cpp
 * @brief Menu definitions.
 */

#include "App/Menu.hpp"

#include <string>
#include <utility>
#include <vector>

namespace Application
{

Menu::Menu(int triggerMod,
           int triggerVk,
           bool executeOnRelease,
           bool exitOnAction,
           bool centerMouseOnOpen,
           bool restoreMouseOnClose,
           std::string name,
           std::vector<std::string> actionIds,
           std::string id)
    : m_triggerMod(triggerMod)
    , m_triggerVk(triggerVk)
    , m_executeOnRelease(executeOnRelease)
    , m_exitOnAction(exitOnAction)
    , m_centerMouseOnOpen(centerMouseOnOpen)
    , m_restoreMouseOnClose(restoreMouseOnClose)
    , m_name(std::move(name))
    , m_id(std::move(id))
    , m_actionIds(std::move(actionIds))
{
}

Menu::~Menu() = default;

void Menu::addActionId(int index, const std::string &actionId)
{
    if (index < 0 || index >= static_cast<int>(m_actionIds.size()))
    {
        m_actionIds.push_back(actionId);
        return;
    }

    m_actionIds.insert(m_actionIds.begin() + index, actionId);
}

void Menu::removeAction(int index)
{
    if (index >= 0 && index < static_cast<int>(m_actionIds.size()))
    {
        m_actionIds.erase(m_actionIds.begin() + index);
    }
}

int Menu::actionCount() const
{
    return static_cast<int>(m_actionIds.size());
}

std::string Menu::id() const
{
    return m_id;
}

std::string Menu::name() const
{
    return m_name;
}

void Menu::setId(const std::string &newId)
{
    m_id = newId;
}

void Menu::setName(const std::string &newName)
{
    m_name = newName;
}

const std::vector<std::string> &Menu::actionIds() const
{
    return m_actionIds;
}

std::string Menu::actionId(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_actionIds.size()))
    {
        return "";
    }

    return m_actionIds[index];
}

void Menu::setActionId(int index, const std::string &actionId)
{
    if (index >= 0 && index < static_cast<int>(m_actionIds.size()))
    {
        m_actionIds[index] = actionId;
    }
}

void Menu::moveActionId(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || toIndex < 0 || fromIndex >= static_cast<int>(m_actionIds.size())
        || toIndex >= static_cast<int>(m_actionIds.size()) || fromIndex == toIndex)
    {
        return;
    }

    std::string actionId = m_actionIds[fromIndex];
    m_actionIds.erase(m_actionIds.begin() + fromIndex);
    m_actionIds.insert(m_actionIds.begin() + toIndex, std::move(actionId));
}

int Menu::triggerMod() const noexcept
{
    return m_triggerMod;
}

void Menu::setTriggerMod(int mod) noexcept
{
    m_triggerMod = mod;
}

int Menu::triggerVk() const noexcept
{
    return m_triggerVk;
}

void Menu::setTriggerVk(int vk) noexcept
{
    m_triggerVk = vk;
}

bool Menu::executeOnRelease() const noexcept
{
    return m_executeOnRelease;
}

void Menu::setExecuteOnRelease(bool enabled) noexcept
{
    m_executeOnRelease = enabled;
}

bool Menu::exitOnAction() const noexcept
{
    return m_exitOnAction;
}

void Menu::setExitOnAction(bool enabled) noexcept
{
    m_exitOnAction = enabled;
}

bool Menu::centerMouseOnOpen() const noexcept
{
    return m_centerMouseOnOpen;
}

void Menu::setCenterMouseOnOpen(bool enabled) noexcept
{
    m_centerMouseOnOpen = enabled;
}

bool Menu::restoreMouseOnClose() const noexcept
{
    return m_restoreMouseOnClose;
}

void Menu::setRestoreMouseOnClose(bool enabled) noexcept
{
    m_restoreMouseOnClose = enabled;
}

} // namespace Application
