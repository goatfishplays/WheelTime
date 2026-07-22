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
    Menu(int triggerMod = 0,
         int triggerVk = 0,
         bool executeOnRelease = false,
         bool exitOnAction = false,
         bool centerMouseOnOpen = true,
         bool restoreMouseOnClose = false,
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
    [[nodiscard]] std::string id() const;
    [[nodiscard]] std::string name() const;
    void setId(const std::string &newId);
    void setName(const std::string &newName);
    /// @brief Ordered action references used as wheel slots.
    [[nodiscard]] const std::vector<std::string> &actionIds() const;
    [[nodiscard]] std::string actionId(int index) const;
    void setActionId(int index, const std::string &actionId);
    /// @brief Reorders a slot reference inside this menu.
    void moveActionId(int fromIndex, int toIndex);

    [[nodiscard]] int triggerMod() const noexcept;
    void setTriggerMod(int mod) noexcept;
    [[nodiscard]] int triggerVk() const noexcept;
    void setTriggerVk(int vk) noexcept;

    /// @brief On hotkey release, execute the action under the mouse.
    [[nodiscard]] bool executeOnRelease() const noexcept;
    void setExecuteOnRelease(bool enabled) noexcept;

    /// @brief Exit the menu after performing an action.
    [[nodiscard]] bool exitOnAction() const noexcept;
    void setExitOnAction(bool enabled) noexcept;

    /// @brief Move the cursor to the monitor center when this menu opens.
    [[nodiscard]] bool centerMouseOnOpen() const noexcept;
    void setCenterMouseOnOpen(bool enabled) noexcept;

    /// @brief Snap the cursor back to its pre-open position when this menu closes.
    [[nodiscard]] bool restoreMouseOnClose() const noexcept;
    void setRestoreMouseOnClose(bool enabled) noexcept;

private:
    int m_triggerMod = 0;
    int m_triggerVk = 0;
    bool m_executeOnRelease = false;
    bool m_exitOnAction = false;
    bool m_centerMouseOnOpen = true;
    bool m_restoreMouseOnClose = false;
    std::string m_name;
    std::string m_id;
    std::vector<std::string> m_actionIds;
};

} // namespace Application
