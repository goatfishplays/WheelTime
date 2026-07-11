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
             std::vector<Action> _actions = {});
        ~Menu();

        /**
         * @brief Add an action to this menu
         *
         * @param ind Index of action to add
         * @param action Action to add
         */
        void addAction(int ind, Action action);

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
        int numActions();

        /**
         * @brief Saves menu/settings to a file
         *
         * @param filepath file to save to
         */
        void save();

        /**
         * @brief Loads menu/settings from a file
         *
         * @param filepath file to load from
         */
        void load();

        std::string getName() const;

        /// @brief Generally avoid directly modifying this
        std::vector<Action> actions;

    private:
        std::string name;
        std::string filepath; // TODO: need to auto delete old on change?
    };
}