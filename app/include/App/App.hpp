/**
 * @file App.hpp
 * @author your name (you@domain.com)
 * @brief Contains `App` class, controls state of app
 * @version 0.1
 * @date 2026-06-29
 *
 * @copyright Copyright (c) 2026
 *
 */
// TODO: InputBind struct that contains mod and stuff
#pragma once
#include <QApplication>
#include <vector>
#include <Platform/Window.hpp>
#include <Platform/Execute.hpp>
#include <Platform/Inputs.hpp>
#include "App/Menu.hpp"
#include "App/Gui.hpp"

namespace Application
{
    /**
     * @brief Controls the current state of the application and visible window
     */
    class App
    {
    public:
        /// @brief All currently cached/loaded menus TODO: likely want to keep a timer/lifetime + lru combo for menus or just load all
        std::vector<Menu *> loadedMenus;

        Platform::InputRcvr inputRcvr; // TODO: can mbe make these classes have a lot of static funcs for perf later
        Platform::Executor executor;
        Gui gui;

        // Singleton
        App(const App &) = delete;
        App &operator=(const App &) = delete;
        App(App &&) = delete;
        App &operator=(App &&) = delete;

        static App &getInstance()
        {
            static App instance;
            return instance;
        }

        /// @brief Gets the active Menu
        /// @return Pointer to the active menu
        Menu *getActiveMenu();

        /// @brief Set, display, and focus a menu
        /// @param menu to be displayed
        void showGui(Menu *menu);

        /// @brief Exits the active menu and returns to previous window (or newly open window? might need to look into this)
        void hideGui();

        /// @brief Run an action from current menu
        /// @param actionInd
        void executeAction(int actionInd);

        /// @brief Callback when global hotkey is pressed
        void onHotkeyTriggered(int hotkeyId);

    private:
        /// @brief The currently viewed menu
        Menu *activeMenu;
        Platform::Vec2 priorMousePos;
        Platform::Window priorWindow;

        /**
         * @brief Collects the place to return to after GUI closes
         *
         */
        void gatherPriors();
        /**
         * @brief Returns to the window and mouse position saved from gatherPrior()
         *
         */
        void restorePriors();
        App();
        ~App();
        Platform::InputRcvr m_inputRcvr;
        class QAbstractNativeEventFilter *m_hotkeyFilter;
    };

}