/**
 * @file App.hpp
 * @brief Contains `App` class, controls state of app
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
    class SettingsWindow;

    class App
    {
    public:
        std::vector<Menu *> loadedMenus;
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

        Menu *getActiveMenu();
        Menu *findMenuById(const std::string &menuId);
        const Menu *findMenuById(const std::string &menuId) const;
        Action *findActionById(const std::string &actionId);
        const Action *findActionById(const std::string &actionId) const;
        std::vector<std::string> getActionLabelsForMenu(const Menu &menu) const;
        std::vector<Menu> getMenuCopies() const;
        const std::vector<Action> &getActionLibrary() const;

        void showGui(Menu *menu);
        void hideGui();
        void executeAction(int actionInd);
        void onHotkeyTriggered(int hotkeyId);
        void showSettingsWindow();
        bool saveConfig();
        bool applyConfig(const std::vector<Action> &actions, const std::vector<Menu> &menus);
        void refreshActiveMenu();
        QString getConfigPath() const;

    private:
        Menu *activeMenu;
        Platform::Vec2 priorMousePos;
        Platform::Window priorWindow;

        void gatherPriors();
        void restorePriors();
        void clearMenus();
        App();
        ~App();

        Platform::InputRcvr m_inputRcvr;
        class QAbstractNativeEventFilter *m_hotkeyFilter;
        SettingsWindow *m_settingsWindow;
        QString m_configPath;
    };
}
