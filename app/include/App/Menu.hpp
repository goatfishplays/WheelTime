/**
 * @file Menu.hpp
 * @brief Radial menu configuration: hotkey bind and ordered action-id slots.
 */

#pragma once

#include <string>
#include <vector>

#include "App/Action.hpp"

namespace Application
{

/**
 * @brief Represents one radial menu configuration.
 *
 * Menus keep an ordered list of action IDs so the same action can be reused in
 * multiple menus and edited centrally from the action library.
 */
class Menu
{
public:
    int triggerMod = 0;
    int triggerVk = 0;

    /// @brief On hotkey release, execute the action under the mouse.
    bool executeOnRelease = false;

    /// @brief Exit the menu after performing an action.
    bool exitOnAction = false;

    Menu(int triggerMod = 0,
         int triggerVk = 0,
         bool executeOnRelease = false,
         bool exitOnAction = false,
         std::string name = "Unnamed Menu",
         std::vector<std::string> actionIds = {},
         std::string id = "");
    ~Menu();

    /// @brief Inserts an action reference at @p index, or appends when out of range.
    void addActionId(int index, const std::string &actionId);

    /// @brief Removes the action reference at @p index when in range.
    void removeAction(int index);

    /// @brief Number of action slots in this menu.
    [[nodiscard]] int actionCount() const;

    /// @brief Stable config ID used by menu references and serialization.
    [[nodiscard]] std::string getId() const;
    [[nodiscard]] std::string getName() const;
    void setId(const std::string &newId);
    void setName(const std::string &newName);
    /// @brief Ordered action references used as wheel slots.
    [[nodiscard]] const std::vector<std::string> &getActionIds() const;
    [[nodiscard]] std::string getActionId(int index) const;
    void setActionId(int index, const std::string &actionId);
    /// @brief Reorders a slot reference inside this menu.
    void moveActionId(int fromIndex, int toIndex);

private:
    std::string m_name;
    std::string m_id;
    std::vector<std::string> m_actionIds;
};

} // namespace Application
