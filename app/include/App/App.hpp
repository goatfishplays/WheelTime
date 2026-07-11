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
#pragma once

#include <QApplication>
#include <QString>
#include <vector>

#include <Platform/Execute.hpp>
#include <Platform/Inputs.hpp>
#include <Platform/Window.hpp>

#include "App/Action.hpp"
#include "App/Gui.hpp"
#include "App/Menu.hpp"

namespace Application
{
    /// @brief Forward declaration for the non-modal settings editor window.
    class SettingsWindow;

    /**
     * @brief Owns the long-lived runtime state for WheelTime.
     *
     * `App` now keeps two related collections:
     * - `actionLibrary`: reusable actions keyed by stable IDs
     * - `loadedMenus`: wheel menus that reference those actions by ID
     *
     * That split lets the same action be reused in multiple menus and keeps
     * the runtime aligned with the JSON config edited from the settings window.
     */
    class App
    {
    public:
        /// @brief Loaded wheel menus. Each menu owns only slot references.
        std::vector<Menu *> loadedMenus;
        /// @brief Shared library of reusable actions referenced by menu slots.
        std::vector<Action> actionLibrary;

        Platform::InputRcvr inputRcvr;
        Platform::Executor executor;
        Gui gui;

        App(const App &) = delete;
        App &operator=(const App &) = delete;
        App(App &&) = delete;
        App &operator=(App &&) = delete;

        static App &getInstance()
        {
            static App instance;
            return instance;
        }

        /// @brief Returns the menu currently shown in the radial UI.
        Menu *getActiveMenu();
        /// @brief Look up a loaded menu by its stable config ID.
        Menu *findMenuById(const std::string &menuId);
        const Menu *findMenuById(const std::string &menuId) const;
        /// @brief Look up a reusable action by its stable config ID.
        Action *findActionById(const std::string &actionId);
        const Action *findActionById(const std::string &actionId) const;
        /// @brief Resolve user-facing labels for the action IDs in a menu.
        std::vector<std::string> getActionLabelsForMenu(const Menu &menu) const;
        /// @brief Build value copies of menus for the settings working copy.
        std::vector<Menu> getMenuCopies() const;
        /// @brief Exposes the shared action library for editor initialization.
        const std::vector<Action> &getActionLibrary() const;

        /// @brief Shows the radial launcher for a specific menu.
        void showGui(Menu *menu);
        /// @brief Hides the radial launcher and restores prior focus/mouse state.
        void hideGui();
        /// @brief Executes the action referenced by the selected slot index.
        void executeAction(int actionInd);
        /// @brief Global hotkey callback used to show or hide the launcher.
        void onHotkeyTriggered(int hotkeyId);
        /// @brief Opens the non-modal settings editor window.
        void showSettingsWindow();
        /// @brief Persists the current runtime config to disk.
        bool saveConfig();
        /// @brief Replaces the live runtime config with an edited working copy.
        bool applyConfig(const std::vector<Action> &actions, const std::vector<Menu> &menus);
        /// @brief Re-renders the active menu after config changes.
        void refreshActiveMenu();
        QString getConfigPath() const;

    private:
        Menu *activeMenu;
        Platform::Vec2 priorMousePos;
        Platform::Window priorWindow;

        void gatherPriors();
        void restorePriors();
        /// @brief Deletes and clears the currently loaded heap-allocated menus.
        void clearMenus();
        App();
        ~App();

        Platform::InputRcvr m_inputRcvr;
        class QAbstractNativeEventFilter *m_hotkeyFilter;
        SettingsWindow *m_settingsWindow;
        /// @brief Absolute path to the JSON config file used for load/save.
        QString m_configPath;
    };
}
