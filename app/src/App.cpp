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
#include <QMetaObject>
#include <QPushButton>
#include <QThread>

#include "App/MenuConfigLoader.hpp"
#include "App/SettingsWindow.hpp"
#include "App/ActionItems.hpp"

#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <string>
#include <thread>
#include <vector>

using namespace Application;

namespace
{

/// @brief True if @p action is a history/cancel helper that must not enter MRU/MFU.
///
/// Recording nth-recent/nth-frequent (or cancel) would make "Most Recent" the most
/// recent launch, so resolving n=1 schedules itself forever.
bool isHistoryMetaAction(const Action &action)
{
    for (const auto &item : action.getItems())
    {
        if (!item)
        {
            continue;
        }
        const ActionItemKind kind = item->kind();
        if (kind == ActionItemKind::NthRecent || kind == ActionItemKind::NthFrequent
            || kind == ActionItemKind::Cancel)
        {
            return true;
        }
    }
    return false;
}

} // namespace

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
      m_configPath(MenuConfigLoader::defaultConfigPath()),
      m_scheduler(std::make_unique<Scheduler>(
          []
          {
              const unsigned hc = std::thread::hardware_concurrency();
              return hc == 0 ? std::size_t{2} : static_cast<std::size_t>(std::max(2u, std::min(hc, 8u)));
          }()))
{
    // Load the repo-local config on startup. If it is missing or malformed,
    // keep the app alive with a tiny fallback menu so the GUI still opens.
    if (!MenuConfigLoader::loadConfig(m_configPath, m_appConfig, actionLibrary, loadedMenus))
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<AI_Close>());
        actionLibrary.push_back(Action(std::move(items), "Config missing", "", "action-config-missing"));
        loadedMenus.push_back(new Menu(nullptr, false, false, "Config Error", {"action-config-missing"}, "menu-config-error"));
    }

    Platform::InputBind bind;
    bind.mod = m_appConfig.globalHotkeyMod;
    bind.input = m_appConfig.globalHotkeyVk;
    m_inputRcvr.registerInputBinding(bind);

    // Install native event filter to capture WM_HOTKEY
    m_hotkeyFilter = new HotkeyFilter(this);
    qApp->installNativeEventFilter(m_hotkeyFilter);

    // Connect escapePressed signal from Gui to hide and return focus
    QObject::connect(&gui, &Gui::escapePressed, [this]()
                     { hideGui(); });

    // Move the old loadConfig block up above so it is loaded before inputRcvr registration

    m_actionHistory.load(historyPath());
    pruneActionHistory();

    if (!loadedMenus.empty())
    {
        activeMenu = loadedMenus.front();
        gui.setMenu(*activeMenu, getActionLabelsForMenu(*activeMenu));
    }

    initializeOverlay();
}

App::~App()
{
    saveActionHistory();

    if (m_scheduler)
    {
        m_scheduler->stop();
    }

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

    // Unregister hotkey
    Platform::InputBind bind;
    bind.mod = m_appConfig.globalHotkeyMod;
    bind.input = m_appConfig.globalHotkeyVk;
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
    // Don't fight the settings editor (overlay shell is parked while it is open).
    if (m_settingsWindow != nullptr && m_settingsWindow->isVisible())
    {
        m_settingsWindow->raise();
        m_settingsWindow->activateWindow();
        return;
    }

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

    // ActionItems run on scheduler worker threads; Qt widgets must only be touched
    // on the GUI thread.
    if (QCoreApplication::instance() != nullptr
        && QThread::currentThread() != QCoreApplication::instance()->thread())
    {
        const std::string menuId = menu->getId();
        QMetaObject::invokeMethod(
            QCoreApplication::instance(),
            [this, menuId]()
            {
                showGui(findMenuById(menuId));
            },
            Qt::QueuedConnection);
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
    if (QCoreApplication::instance() != nullptr
        && QThread::currentThread() != QCoreApplication::instance()->thread())
    {
        QMetaObject::invokeMethod(
            QCoreApplication::instance(),
            [this]()
            {
                hideGui();
            },
            Qt::QueuedConnection);
        return;
    }

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

    // Wheel launches own usage history (not nested scheduleAction / nth-* re-runs).
    // Skip history/cancel helpers so "Most Recent" cannot become most recent.
    if (!isHistoryMetaAction(*action))
    {
        m_actionHistory.recordUse(action->getId());
        saveActionHistory();
    }

    // Copy into the scheduler so the library Action stays editable/reusable.
    m_scheduler->submit(*action);

    if (activeMenu->exitOnAction && gui.isVisible())
    {
        hideGui();
    }
}

Action *App::nthRecentAction(int n)
{
    if (n < 1)
    {
        return nullptr;
    }

    // Walk raw MRU ranks, skipping helpers / missing ids, until the Nth real Action.
    int found = 0;
    for (int rank = 1;; ++rank)
    {
        const std::string id = m_actionHistory.nthRecentId(rank);
        if (id.empty())
        {
            return nullptr;
        }
        Action *candidate = findActionById(id);
        if (candidate == nullptr || isHistoryMetaAction(*candidate))
        {
            continue;
        }
        if (++found == n)
        {
            return candidate;
        }
    }
}

Action *App::nthFrequentAction(int n)
{
    if (n < 1)
    {
        return nullptr;
    }

    // Walk frequency ranks the same way so meta Actions never win MFU either.
    int found = 0;
    for (int rank = 1;; ++rank)
    {
        const std::string id = m_actionHistory.nthFrequentId(rank);
        if (id.empty())
        {
            return nullptr;
        }
        Action *candidate = findActionById(id);
        if (candidate == nullptr || isHistoryMetaAction(*candidate))
        {
            continue;
        }
        if (++found == n)
        {
            return candidate;
        }
    }
}

QString App::historyPath() const
{
    const QFileInfo configInfo(m_configPath);
    return configInfo.dir().filePath("action_history.json");
}

void App::pruneActionHistory()
{
    std::vector<std::string> ids;
    ids.reserve(actionLibrary.size());
    for (const Action &action : actionLibrary)
    {
        if (!action.getId().empty())
        {
            ids.push_back(action.getId());
        }
    }
    m_actionHistory.pruneToLibrary(ids);
}

void App::saveActionHistory()
{
    m_actionHistory.save(historyPath());
}

Scheduler &App::scheduler() noexcept
{
    return *m_scheduler;
}

const Scheduler &App::scheduler() const noexcept
{
    return *m_scheduler;
}

void App::showSettingsWindow()
{
    if (gui.isLauncherVisible())
    {
        hideGui();
    }

    // Soft-park the overlay: keep the Qt widget alive (no native SW_HIDE — that
    // desyncs QWidget visibility and later crashes showGui), but clear topmost so
    // the settings window can stay above it.
    if (m_overlayInitialized)
    {
        Platform::Window overlayWindow(reinterpret_cast<void *>(gui.winId()));
        overlayWindow.setClickThrough(true);
        overlayWindow.setTopmost(false);
    }

    if (m_settingsWindow == nullptr)
    {
        m_settingsWindow = new SettingsWindow();
        QObject::connect(m_settingsWindow, &SettingsWindow::saveRequested, [this]()
                         {
                             // The settings window edits a detached working copy.
                             // after the user explicitly saves.
                             AppConfig newConfig;
                             std::vector<Action> newActions;
                             std::vector<Menu> newMenus;
                             m_settingsWindow->exportWorkingCopy(newConfig, newActions, newMenus);
                             applyConfig(newConfig, newActions, newMenus); });

        QObject::connect(m_settingsWindow, &SettingsWindow::windowClosed, [this]()
                         {
                             restoreOverlayAfterSettings();
                             if (m_scheduler)
                             {
                                 m_scheduler->resume();
                             } });
    }

    // Pause macros while editing; resume when the window closes.
    m_scheduler->pause();
    m_settingsWindow->loadWorkingCopy(m_appConfig, actionLibrary, getMenuCopies());
    m_settingsWindow->show();
    m_settingsWindow->raise();
    m_settingsWindow->activateWindow();
}

void App::restoreOverlayAfterSettings()
{
    if (!m_overlayInitialized)
    {
        return;
    }

    // Re-apply the dormant overlay shell styles. Avoid show()/hide() mismatches
    // with Qt by only using showNoActivate after styles are restored.
    gui.enterDormantOverlay();
    Platform::Window overlayWindow(reinterpret_cast<void *>(gui.winId()));
    overlayWindow.setTransparentOverlay(true);
    overlayWindow.setNonActivating(true);
    overlayWindow.setClickThrough(true);
    overlayWindow.setTopmost(true);
    overlayWindow.showNoActivate();
}

bool App::saveConfig()
{
    return MenuConfigLoader::saveConfig(m_configPath, m_appConfig, actionLibrary, loadedMenus);
}

bool App::applyConfig(const AppConfig &appConfig, const std::vector<Action> &actions, const std::vector<Menu> &menus)
{
    // Unregister old hotkey
    Platform::InputBind oldBind;
    oldBind.mod = m_appConfig.globalHotkeyMod;
    oldBind.input = m_appConfig.globalHotkeyVk;
    m_inputRcvr.unregisterInputBinding(oldBind);

    m_appConfig = appConfig;

    // Register new hotkey
    Platform::InputBind newBind;
    newBind.mod = m_appConfig.globalHotkeyMod;
    newBind.input = m_appConfig.globalHotkeyVk;
    m_inputRcvr.registerInputBinding(newBind);

    // Drop in-flight macros that may reference old library Actions / menus.
    if (m_scheduler)
    {
        m_scheduler->cancelAll();
    }
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
    pruneActionHistory();
    saveActionHistory();
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
