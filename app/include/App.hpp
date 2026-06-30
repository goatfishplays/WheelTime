/**
 * @file App.hpp
 * @author your name (you@domain.com)
 * @brief The action application itself
 * @version 0.1
 * @date 2026-06-29
 *
 * @copyright Copyright (c) 2026
 *
 */
// TODO: Hotkey struct that contains mod and stuff
#pragma once
#include <string>
#include <vector>
#include <Platform/Inputs.hpp>
#include <Platform/Window.hpp>

namespace App
{
    /**
     * @brief Controls the current state of the application and visible window
     */
    class App
    {
    public:
        App();
        ~App();
        /// @brief All currently cached/loaded menus TODO: likely want to keep a timer/lifetime + lru combo for menus or just load all
        std::vector<Menu *> loadedMenus;

        /// @brief Gets the active Menu
        /// @return Pointer to the active menu
        Menu *getActiveMenu();

        /// @brief Set the active menu
        /// @param menu
        void setActiveMenu(Menu *menu);

        /// @brief Exits the active menu and returns to previous window (or newly open window? might need to look into this)
        void exitMenu();

        /// @brief Run an action
        /// @param action
        void runAction(Action action);

    private:
        /// @brief The currently viewed menu
        Menu *activeMenu;

        Platform::Vec2 priorMousePos;
        Platform::Window window;
    };

    class Menu
    {
    public:
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
        void save(std::string filepath);

        /**
         * @brief Loads menu/settings from a file
         *
         * @param filepath file to load from
         */
        void load(std::string filepath);

        /// @brief Hotkey to trigger this menu, if not set then this will likely only be being used as a recursive menu
        // TODO: Might be better to have app control hotkeys
        Platform::Hotkey *hotkey;

        /// @brief On hotkey release will automatically execute the action under the mouse
        bool executeOnRelease;

        /// @brief Automatically exit the menu on performing an action(as opposed to individual actions requiring exit, allows people to spam buttons easier)
        bool exitOnAction;

    private:
        std::vector<Action> actions;
    };

    /**
     * @brief Contains a sequence of actions to be executed
     *
     */
    class Action
    {
    public:
        /**
         * @brief Add an item from the sequence
         *
         * @param ind Index of item to add
         * @param ai Action item to add
         */
        void addItem(int ind, ActionItem &ai);

        /**
         * @brief Remove an item from the sequence
         *
         * @param ind Index of item to remove
         */
        void removeItem(int ind);

        /**
         * @brief Gets the number of `ActionItem`s in the sequence
         *
         * @return int
         */
        int len();

        /**
         * @brief Runs the action items in sequence
         *
         */
        void execute();

    private:
        std::vector<ActionItem> sequence;
    };

    /**
     * @brief Represents an individual action to be taken
     *
     * Can be extended via inheritance
     *
     */
    class ActionItem
    {
    public:
        ActionItem();
        ~ActionItem();
        /**
         * @brief To be overwritten performs the said action
         *
         */
        virtual void execute() = 0;
    };

    /**
     * @brief Runs a specified script
     *
     */
    class AI_Script : public ActionItem
    {
    public:
        std::string filepath;
        void execute() override;
    };

    /**
     * @brief Runs a specified keystroke
     *
     */
    class AI_Keystroke : public ActionItem
    {
    public:
        int keycode;
        int modifiers;
        float holdDuration;
        /// @brief Continue to next `ActionItem` immediately if `true` else, waits till keystroke finished
        // * This isn't needed from a technical standpoint as it can be accomplished with the delay AI but from a user standpoint it should be helpful
        bool proceed;
        void execute() override;
    };

    /**
     * @brief Delays the next action in sequence
     *
     */
    class AI_Delay : public ActionItem
    {
    public:
        float duration;
        void execute() override;
    };

    /**
     * @brief Opens a submenu
     *
     */
    class AI_Submenu : public ActionItem
    {
    public:
        std::string settingsFilepath;
        void execute() override;
    };

    /**
     * @brief Closes the menu
     *
     */
    class AI_Close : public ActionItem
    {
    public:
        void execute() override;
    };

    /**
     * @brief Sends a socket message
     *
     */
    class AI_Close : public ActionItem
    {
    public:
        std::string socketMsg;
        std::string outputDst;
        void execute() override;
    };
}