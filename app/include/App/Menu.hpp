/**
 * @file Menu.hpp
 * @brief Declares the wheel-menu model used by the launcher and settings UI.
 */

#pragma once
#include <string>
#include <vector>
#include <Platform/Inputs.hpp>
#include "App/Action.hpp"

namespace Application
{
    /**
     * @brief Represents one radial menu configuration.
     *
     * Menus no longer own full `Action` objects. Instead they keep an ordered
     * list of action IDs so the same action can be reused in multiple menus and
     * edited centrally from the action library.
     */
    class Menu
    {
    public:
        /// @brief InputBind to trigger this menu, if not set then this will likely only be being used as a recursive menu
        // TODO: Might be better to have app control hotkeys
        Platform::InputBind *inputBind;

        /// @brief On hotkey release will automatically execute the action under the mouse
        bool executeOnRelease;

        /// @brief Automatically exit the menu on performing an action(as opposed to individual actions requiring exit, allows people to spam buttons easier)
        // TODO just make it so that it prepends exit if true
        bool exitOnAction;

        Menu(Platform::InputBind *_inputBind = nullptr,
             bool _executeOnRelease = false,
             bool _exitOnAction = false,
             std::string name = "Unnamed Menu",
             std::vector<std::string> _actionIds = {},
             std::string id = "");
        ~Menu();

        /**
         * @brief Inserts an action reference into the slot list.
         *
         * @param ind Index to insert at, or append when out of range
         * @param actionId Stable ID of the action to reference
         */
        void addActionId(int ind, const std::string &actionId);

        /**
         * @brief Remove an action from this menu
         *
         * @param ind Index of action to remove
         */
        void remAction(int ind);

        /**
         * @brief Gets the number of actions in this menu
         *
         * @return int
         */
        int numActions() const;

        /// @brief Stable config ID used by menu references and serialization.
        std::string getId() const;
        std::string getName() const;
        void setId(const std::string &newId);
        void setName(const std::string &newName);
        /// @brief Ordered action references used as wheel slots.
        const std::vector<std::string> &getActionIds() const;
        std::string getActionId(int index) const;
        void setActionId(int index, const std::string &actionId);
        /// @brief Reorders a slot reference inside this menu.
        void moveActionId(int fromIndex, int toIndex);

    private:
        std::string name;
        std::string id;
        std::vector<std::string> actionIds;
    };
}
