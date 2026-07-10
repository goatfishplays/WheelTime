/**
 * @file Menu.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once
#include <string>
#include <vector>
#include <Platform/Inputs.hpp>
#include "App/Action.hpp"

namespace Application
{

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
         * @brief Add an action to this menu
         *
         * @param ind Index of action to add
         * @param action Action to add
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

        std::string getId() const;
        std::string getName() const;
        void setId(const std::string &newId);
        void setName(const std::string &newName);
        const std::vector<std::string> &getActionIds() const;
        std::string getActionId(int index) const;
        void setActionId(int index, const std::string &actionId);
        void moveActionId(int fromIndex, int toIndex);

    private:
        std::string name;
        std::string id;
        std::vector<std::string> actionIds;
    };
}
