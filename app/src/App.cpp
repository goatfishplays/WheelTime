/**
 * @file App.cpp
* @author goatfishplays@gmail.com
 * @brief Contains the implementation of the actual app
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026
 */

#include "App/App.hpp"

#include <Platform/Window.hpp>
#include <Platform/Inputs.hpp>
#include <Platform/Execute.hpp>
#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QPushButton>

#include "App/MenuConfigLoader.hpp"
#include "App/SettingsWindow.hpp"

using namespace Application;

class HotkeyFilter : public QAbstractNativeEventFilter
{
public:
    explicit HotkeyFilter(App *app) : m_app(app) {}

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override
    {
        int hotkeyId = 0;
        if (Platform::InputRcvr::isHotkeyMessage(message, hotkeyId))
        {
            m_app->onHotkeyTriggered(hotkeyId);
            return true; // Handled
        }
        return false;
    }

private:
    App *m_app;
};

App::App()
    : activeMenu(nullptr),
      gui(0),
      priorMousePos{},
      priorWindow{},
      inputRcvr(),
      executor(),
      m_hotkeyFilter(nullptr),
      m_settingsWindow(nullptr),
      m_configPath(MenuConfigLoader::defaultConfigPath())
{
    Platform::InputBind bind;
    bind.mod = 0x0001; // MOD_ALT
    bind.input = 0x20; // VK_SPACE
    m_inputRcvr.registerInputBinding(bind);

    // Install native event filter to capture WM_HOTKEY
    m_hotkeyFilter = new HotkeyFilter(this);
    qApp->installNativeEventFilter(m_hotkeyFilter);

    // Connect escapePressed signal from Gui to hide and return focus
    QObject::connect(&gui, &Gui::escapePressed, [this]()
                     { hideGui(); });

    // Load the repo-local config on startup. If it is missing or malformed,
    // keep the app alive with a tiny fallback menu so the GUI still opens.
    if (!MenuConfigLoader::loadConfig(m_configPath, actionLibrary, loadedMenus))
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<AI_Close>());
        actionLibrary.push_back(Action(std::move(items), "Config missing", "", "action-config-missing"));
        loadedMenus.push_back(new Menu(nullptr, false, false, "Config Error", {"action-config-missing"}, "menu-config-error"));
    }

    if (!loadedMenus.empty())
    {
        activeMenu = loadedMenus.front();
        gui.setMenu(*activeMenu, getActionLabelsForMenu(*activeMenu));
    }

    initializeOverlay();
}

App::~App()
{
    if (m_hotkeyFilter)
    {
        qApp->removeNativeEventFilter(m_hotkeyFilter);
        delete m_hotkeyFilter;
        m_hotkeyFilter = nullptr;
    }

    if (m_settingsWindow != nullptr)
    {
        delete m_settingsWindow;
        m_settingsWindow = nullptr;
    }

    clearMenus();

    // Unregister hotkey Alt + Space (mod: 0x0001, vk: 0x20)
    Platform::InputBind bind;
    bind.mod = 0x0001;
    bind.input = 0x20;
    m_inputRcvr.unregisterInputBinding(bind);
}

void App::clearMenus()
{
    for (int i = static_cast<int>(loadedMenus.size()) - 1; i >= 0; --i)
    {
        delete loadedMenus[i];
    }
    loadedMenus.clear();
}

void App::onHotkeyTriggered(int hotkeyId)
{
    if (gui.isLauncherVisible())
    {
        hideGui();
    }
    else
    {
        showGui(activeMenu == nullptr && !loadedMenus.empty() ? loadedMenus.front() : activeMenu);
    }
}

Menu *App::getActiveMenu()
{
    return activeMenu;
}

Menu *App::findMenuById(const std::string &menuId)
{
    for (Menu *menu : loadedMenus)
    {
        if (menu != nullptr && menu->getId() == menuId)
        {
            return menu;
        }
    }
    return nullptr;
}

const Menu *App::findMenuById(const std::string &menuId) const
{
    for (const Menu *menu : loadedMenus)
    {
        if (menu != nullptr && menu->getId() == menuId)
        {
            return menu;
        }
    }
    return nullptr;
}

Action *App::findActionById(const std::string &actionId)
{
    for (Action &action : actionLibrary)
    {
        if (action.getId() == actionId)
        {
            return &action;
        }
    }
    return nullptr;
}

const Action *App::findActionById(const std::string &actionId) const
{
    for (const Action &action : actionLibrary)
    {
        if (action.getId() == actionId)
        {
            return &action;
        }
    }
    return nullptr;
}

std::vector<std::string> App::getActionLabelsForMenu(const Menu &menu) const
{
    std::vector<std::string> labels;
    labels.reserve(menu.numActions());

    for (const std::string &actionId : menu.getActionIds())
    {
        const Action *action = findActionById(actionId);
        labels.push_back(action != nullptr ? action->getName() : "Missing Action");
    }

    return labels;
}

std::vector<Menu> App::getMenuCopies() const
{
    std::vector<Menu> menus;
    menus.reserve(loadedMenus.size());
    for (const Menu *menu : loadedMenus)
    {
        if (menu != nullptr)
        {
            menus.push_back(*menu);
        }
    }
    return menus;
}

const std::vector<Action> &App::getActionLibrary() const
{
    return actionLibrary;
}

void App::gatherPriors()
{
    priorMousePos = inputRcvr.getAbsoluteMousePosition();
    priorWindow.getActiveWindow();
}

void App::restorePriors()
{
    // The non-activating overlay should leave keyboard focus where it already
    // is, so we intentionally avoid restoring focus here. Mouse restore can be
    // revisited later if users want the cursor snapped back after overlay use.
}

void App::initializeOverlay()
{
    if (m_overlayInitialized)
    {
        return;
    }

    // Give Qt the same initial fullscreen bounds the native overlay window
    // will use so layered painting has a valid backing-store size from the
    // beginning instead of the default tiny top-level widget size.
    const Platform::Vec2 cursorPos = inputRcvr.getAbsoluteMousePosition();
    Platform::Window overlayWindow(reinterpret_cast<void *>(gui.winId()));
    const Platform::WindowRect bounds = overlayWindow.monitorBoundsForPoint(cursorPos.x, cursorPos.y);
    gui.setGeometry(bounds.x, bounds.y, bounds.width, bounds.height);
    gui.show();
    overlayWindow.setTransparentOverlay(true);
    overlayWindow.setNonActivating(true);
    overlayWindow.setTopmost(true);
    overlayWindow.setBounds(bounds);
    overlayWindow.showNoActivate();
    gui.enterDormantOverlay();
    overlayWindow.setClickThrough(true);
    m_overlayInitialized = true;
}

void App::configureOverlayForCursor()
{
    const Platform::Vec2 cursorPos = inputRcvr.getAbsoluteMousePosition();
    Platform::Window overlayWindow(reinterpret_cast<void *>(gui.winId()));
    const Platform::WindowRect bounds = overlayWindow.monitorBoundsForPoint(cursorPos.x, cursorPos.y);
    // Keep Qt's widget geometry in sync with the native overlay bounds. The
    // shell widget owns the centered panel layout, so it must know its actual
    // monitor-sized rect for rendering and hit-testing to behave correctly.
    gui.setGeometry(bounds.x, bounds.y, bounds.width, bounds.height);
    overlayWindow.setBounds(bounds);
    overlayWindow.setTransparentOverlay(true);
    overlayWindow.setNonActivating(true);
    overlayWindow.setTopmost(true);
}

void App::showGui(Menu *menu)
{
    if (menu == nullptr)
    {
        return;
    }

    initializeOverlay();
    gatherPriors();
    activeMenu = menu;
    gui.setMenu(*activeMenu, getActionLabelsForMenu(*activeMenu));
    configureOverlayForCursor();

    Platform::Window overlayWindow(reinterpret_cast<void *>(gui.winId()));
    overlayWindow.setClickThrough(false);
    overlayWindow.showNoActivate();
    gui.enterInteractiveOverlay();
}

void App::hideGui()
{
    initializeOverlay();
    gui.enterDormantOverlay();
    Platform::Window overlayWindow(reinterpret_cast<void *>(gui.winId()));
    overlayWindow.setClickThrough(true);
    overlayWindow.showNoActivate();
    restorePriors();
}

void App::executeAction(int actionInd)
{
    if (activeMenu == nullptr || actionInd < 0 || actionInd >= activeMenu->numActions())
    {
        return;
    }

    Action *action = findActionById(activeMenu->getActionId(actionInd));
    if (action == nullptr)
    {
        return;
    }

    action->execute();
    if (activeMenu->exitOnAction && gui.isVisible())
    {
        hideGui();
    }
}

void App::showSettingsWindow()
{
    if (gui.isLauncherVisible())
    {
        hideGui();
    }

    if (m_settingsWindow == nullptr)
    {
        m_settingsWindow = new SettingsWindow();
        QObject::connect(m_settingsWindow, &SettingsWindow::saveRequested, [this]()
                         {
                             std::vector<Action> newActions;
                             std::vector<Menu> newMenus;
                             // The settings window edits a detached working copy.
                             // Pull that copy back into the live runtime only
                             // after the user explicitly saves.
                             m_settingsWindow->exportWorkingCopy(newActions, newMenus);
                             applyConfig(newActions, newMenus); });
    }

    m_settingsWindow->loadWorkingCopy(actionLibrary, getMenuCopies());
    m_settingsWindow->show();
    m_settingsWindow->raise();
    m_settingsWindow->activateWindow();
}

bool App::saveConfig()
{
    return MenuConfigLoader::saveConfig(m_configPath, actionLibrary, loadedMenus);
}

bool App::applyConfig(const std::vector<Action> &actions, const std::vector<Menu> &menus)
{
    const std::string previousActiveMenuId = activeMenu == nullptr ? "" : activeMenu->getId();

    // Swap the full runtime model in one step so menus and the shared action
    // library stay in sync after settings edits or schema migration.
    actionLibrary = actions;
    clearMenus();
    loadedMenus.reserve(menus.size());
    for (const Menu &menu : menus)
    {
        loadedMenus.push_back(new Menu(menu));
    }

    activeMenu = previousActiveMenuId.empty() ? nullptr : findMenuById(previousActiveMenuId);
    if (activeMenu == nullptr && !loadedMenus.empty())
    {
        activeMenu = loadedMenus.front();
    }

    const bool saved = saveConfig();
    refreshActiveMenu();
    return saved;
}

void App::refreshActiveMenu()
{
    if (activeMenu != nullptr)
    {
        gui.setMenu(*activeMenu, getActionLabelsForMenu(*activeMenu));
    }
}

QString App::getConfigPath() const
{
    return m_configPath;
}
