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
#include "App/Menu.hpp"

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

        App(int &argc, char **argv);
        ~App();

        /// @brief Gets the active Menu
        /// @return Pointer to the active menu
        Menu *getActiveMenu();

        /// @brief Set the active menu
        /// @param menu
        void setActiveMenu(Menu &menu);

        /// @brief Exits the active menu and returns to previous window (or newly open window? might need to look into this)
        void exitMenu();

        /// @brief Run an action
        /// @param action
        void runAction(Action &action);

    private:
        /// @brief The currently viewed menu
        Menu *activeMenu;

        Platform::Vec2 priorMousePos;
        Platform::Window priorWindow;
        QApplication app;
    };

}