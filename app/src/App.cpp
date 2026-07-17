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
#include <QString>
#include <vector>
#include <QTimer>
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

/// @brief True if @p action is a history/cancel helper that must not enter MRU/MFU.
///
/// Recording nth-recent/nth-frequent (or cancel) would make "Most Recent" the most
/// recent launch, so resolving n=1 schedules itself forever.
bool App::isHistoryMetaAction(const Action &action)
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
        loadedMenus.push_back(new Menu(0, 0, false, false, true, "Config Error", {"action-config-missing"}, "menu-config-error"));
    }

    bool anyHotkey = false;
    for (Menu* m : loadedMenus) {
        if (m->triggerMod == 0 && m->triggerVk == 0) continue;
        anyHotkey = true;
        Platform::InputBind bind;
        bind.mod = m->triggerMod;
        bind.input = m->triggerVk;
        m_inputRcvr.registerInputBinding(bind);
    }

    // Install native event filter to capture WM_HOTKEY
    m_hotkeyFilter = new HotkeyFilter(this);
    qApp->installNativeEventFilter(m_hotkeyFilter);

    // Connect escapePressed signal from Gui to hide and return focus
    QObject::connect(&gui, &Gui::escapePressed, [this]()
                     {
                         if (gui.isSearchVisible())
                         {
                             hideSearchOverlay();
                             return;
                         }
                         hideGui();
                     });

    // Move the old loadConfig block up above so it is loaded before inputRcvr registration

    m_actionHistory.load(historyPath());
    pruneActionHistory();

    if (!loadedMenus.empty())
    {
        activeMenu = loadedMenus.front();
        gui.setMenu(*activeMenu, getActionSlotVisualsForMenu(*activeMenu));
    }

    initializeOverlay();

    // Warm the search palette's program index in the background so the first
    // palette open does not block on the initial shortcut-folder scan.
    gui.preloadSearchIndex();

    m_releaseWatchTimer = new QTimer(qApp);
    m_releaseWatchTimer->setInterval(16);
    QObject::connect(m_releaseWatchTimer, &QTimer::timeout, [this]()
                     { onExecuteOnReleaseTick(); });

    QTimer::singleShot(0, [this]() { showSettingsWindow(); });
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
    for (Menu* m : loadedMenus) {
        if (m->triggerMod == 0 && m->triggerVk == 0) continue;
        Platform::InputBind bind;
        bind.mod = m->triggerMod;
        bind.input = m->triggerVk;
        m_inputRcvr.unregisterInputBinding(bind);
    }
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
    // Don't fight the settings editor while the overlay is in focusable editor mode.
    if (gui.isSettingsVisible())
    {
        return;
    }

    // A menu hotkey while the search palette is open exits search into that
    // menu. Restore prior focus first so wheel mode keeps its invariant that
    // the underlying app owns the keyboard.
    if (gui.isSearchVisible())
    {
        hideSearchOverlay();
    }

    int mod = (hotkeyId >> 16) & 0xFFFF;
    int vk = hotkeyId & 0xFFFF;
    Menu *targetMenu = nullptr;
    for (Menu *m : loadedMenus)
    {
        if (m->triggerMod == mod && m->triggerVk == vk)
        {
            targetMenu = m;
            break;
        }
    }

    // Second press / visible launcher always dismisses (and disarms release-watch).
    if (gui.isLauncherVisible())
    {
        hideGui();
        return;
    }

    Menu *menuToShow = targetMenu;
    if (menuToShow == nullptr && activeMenu == nullptr && !loadedMenus.empty())
    {
        menuToShow = loadedMenus.front();
    }
    else if (menuToShow == nullptr)
    {
        menuToShow = activeMenu;
    }

    if (menuToShow == nullptr)
    {
        return;
    }

    activeMenu = menuToShow;
    showGui(activeMenu);

    if (activeMenu->executeOnRelease)
    {
        armExecuteOnRelease(activeMenu->triggerMod, activeMenu->triggerVk);
    }
}

void App::armExecuteOnRelease(int mod, int vk)
{
    m_executeOnReleaseArmed = true;
    m_releaseWatchMod = mod;
    m_releaseWatchVk = vk;
    if (m_releaseWatchTimer != nullptr)
    {
        m_releaseWatchTimer->start();
    }
}

void App::disarmExecuteOnRelease()
{
    m_executeOnReleaseArmed = false;
    m_releaseWatchMod = 0;
    m_releaseWatchVk = 0;
    if (m_releaseWatchTimer != nullptr)
    {
        m_releaseWatchTimer->stop();
    }
}

void App::onExecuteOnReleaseTick()
{
    if (!m_executeOnReleaseArmed)
    {
        return;
    }

    // Wait until primary key AND every launcher modifier are up. Otherwise
    // releasing only Tab on Ctrl+Shift+Tab would inject keys with Ctrl/Shift still held.
    if (!m_inputRcvr.isChordFullyReleased(m_releaseWatchMod, m_releaseWatchVk))
    {
        return;
    }

    disarmExecuteOnRelease();

    if (!gui.isLauncherVisible())
    {
        return;
    }

    gui.refreshSelectionFromCursor();
    const int selected = gui.selectedActionIndex();
    if (activeMenu != nullptr && selected >= 0 && selected < activeMenu->actionCount()
        && findActionById(activeMenu->getActionId(selected)) != nullptr)
    {
        executeAction(selected);
        return;
    }

    // No valid selection — close so a hold-open does not stick after release.
    hideGui();
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

std::vector<ActionSlotVisual> App::getActionSlotVisualsForMenu(const Menu &menu) const
{
    std::vector<ActionSlotVisual> visuals;
    visuals.reserve(menu.actionCount());

    const QFileInfo configInfo(m_configPath);
    const QDir configDir = configInfo.absoluteDir();

    for (const std::string &actionId : menu.getActionIds())
    {
        const Action *action = findActionById(actionId);
        ActionSlotVisual visual;
        visual.label = action != nullptr ? action->getName() : "Missing Action";
        if (action != nullptr && !action->getIconFilepath().empty())
        {
            const QString iconPath = QString::fromStdString(action->getIconFilepath());
            const QFileInfo iconInfo(iconPath);
            if (iconInfo.isAbsolute())
            {
                visual.iconPath = iconPath.toStdString();
            }
            else
            {
                visual.iconPath = configDir.filePath(iconPath).toStdString();
            }
        }
        visuals.push_back(std::move(visual));
    }

    return visuals;
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

    // Wheel and search are mutually exclusive; wheel mode must also win back
    // the non-activating focus model, so restore the prior foreground window.
    if (gui.isSearchVisible())
    {
        hideSearchOverlay();
    }

    initializeOverlay();
    gatherPriors();
    activeMenu = menu;
    // Size the overlay to the current monitor first so radius/layout use final dims.
    configureOverlayForCursor();

    if (activeMenu->centerMouseOnOpen)
    {
        const Platform::Vec2 cursorPos = inputRcvr.getAbsoluteMousePosition();
        Platform::Window overlayWindow(reinterpret_cast<void *>(gui.winId()));
        const Platform::WindowRect bounds =
            overlayWindow.monitorBoundsForPoint(cursorPos.x, cursorPos.y);
        // Start the cursor slightly above the wheel center; scales with the
        // monitor so the nudge feels the same across resolutions.
        const int upwardNudge = bounds.height * 2 / 100;
        inputRcvr.setAbsoluteMousePosition(Platform::Vec2{
            bounds.x + bounds.width / 2,
            bounds.y + bounds.height / 2 - upwardNudge});
    }

    gui.setMenu(*activeMenu, getActionSlotVisualsForMenu(*activeMenu));

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

    disarmExecuteOnRelease();
    initializeOverlay();
    gui.enterDormantOverlay();
    Platform::Window overlayWindow(reinterpret_cast<void *>(gui.winId()));
    overlayWindow.setClickThrough(true);
    overlayWindow.showNoActivate();
    restorePriors();
}

void App::showSearchOverlay(const SearchConfig &config)
{
    // AI_Search runs on scheduler worker threads; Qt widgets must only be
    // touched on the GUI thread.
    if (QCoreApplication::instance() != nullptr
        && QThread::currentThread() != QCoreApplication::instance()->thread())
    {
        QMetaObject::invokeMethod(
            QCoreApplication::instance(),
            [this, config]()
            {
                showSearchOverlay(config);
            },
            Qt::QueuedConnection);
        return;
    }

    // Don't fight the settings editor; search and settings are mutually exclusive.
    if (gui.isSettingsVisible())
    {
        return;
    }

    disarmExecuteOnRelease();
    initializeOverlay();

    // The wheel never takes focus, so even if it is currently up the real
    // foreground window is still the user's app — capture it before we steal
    // focus so hideSearchOverlay can give it back.
    gatherPriors();
    configureOverlayForCursor();

    // Search mode needs real keyboard focus for the query field, unlike wheel
    // mode. Same native flip settings mode uses.
    Platform::Window overlayWindow(reinterpret_cast<void *>(gui.winId()));
    overlayWindow.setNonActivating(false);
    overlayWindow.setClickThrough(false);
    overlayWindow.setTopmost(true);

    // showSearchPanel hides the wheel/settings panels and flips the mode enum.
    gui.showSearchPanel(config);
    gui.show();
    gui.raise();
    gui.activateWindow();
    // Beat the Windows foreground lock if activateWindow was not enough.
    overlayWindow.focus();
    gui.focusSearchInput();
}

void App::hideSearchOverlay(bool restoreFocus)
{
    if (QCoreApplication::instance() != nullptr
        && QThread::currentThread() != QCoreApplication::instance()->thread())
    {
        QMetaObject::invokeMethod(
            QCoreApplication::instance(),
            [this, restoreFocus]()
            {
                hideSearchOverlay(restoreFocus);
            },
            Qt::QueuedConnection);
        return;
    }

    if (!gui.isSearchVisible())
    {
        return;
    }

    gui.hideSearchPanel();

    // Back to the dormant shell: non-activating, click-through, topmost.
    Platform::Window overlayWindow(reinterpret_cast<void *>(gui.winId()));
    overlayWindow.setTransparentOverlay(true);
    overlayWindow.setNonActivating(true);
    overlayWindow.setClickThrough(true);
    overlayWindow.setTopmost(true);
    overlayWindow.showNoActivate();

    // Search genuinely steals focus (unlike the wheel), so hand it back.
    if (restoreFocus)
    {
        priorWindow.focus();
    }
}

void App::executeAction(int actionInd)
{
    // Click-to-execute while holding must not also fire on release.
    disarmExecuteOnRelease();

    if (activeMenu == nullptr || actionInd < 0 || actionInd >= activeMenu->actionCount())
    {
        return;
    }

    Action *action = findActionById(activeMenu->getActionId(actionInd));
    if (action == nullptr)
    {
        return;
    }

    // Clear leftover launcher modifiers before inject (click-while-holding path).
    // Release-execute also waits for a full physical chord-up; this is the safety net.
    if (activeMenu->triggerMod != 0)
    {
        executor.modifiersUp(activeMenu->triggerMod);
    }

    executeActionById(action->getId());

    if (activeMenu->exitOnAction && gui.isLauncherVisible())
    {
        hideGui();
    }
}

void App::executeActionById(const std::string &actionId)
{
    Action *action = findActionById(actionId);
    if (action == nullptr)
    {
        return;
    }

    // Direct launches own usage history (not nested scheduleAction / nth-* re-runs).
    // Skip history/cancel helpers so "Most Recent" cannot become most recent.
    if (!isHistoryMetaAction(*action))
    {
        m_actionHistory.recordUse(action->getId());
        saveActionHistory();
    }

    // Copy into the scheduler so the library Action stays editable/reusable.
    m_scheduler->submit(*action);
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
    // Settings replaces the search palette; settings takes focus itself, so
    // there is no point bouncing focus back to the prior window first.
    if (gui.isSearchVisible())
    {
        hideSearchOverlay(/*restoreFocus=*/false);
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

    initializeOverlay();
    configureOverlayForCursor();

    // Settings mode is intentionally different from wheel mode. The wheel must
    // not steal keyboard focus from games/apps, but the editor has line edits,
    // combo boxes, and hotkey recording controls that need normal focus.
    Platform::Window overlayWindow(reinterpret_cast<void *>(gui.winId()));
    overlayWindow.setTransparentOverlay(true);
    overlayWindow.setNonActivating(false);
    overlayWindow.setClickThrough(false);
    overlayWindow.setTopmost(true);

    // Pause macros while editing; resume when the window closes.
    m_scheduler->pause();
    m_settingsWindow->loadWorkingCopy(m_appConfig, actionLibrary, getMenuCopies());
    gui.showSettingsPanel(m_settingsWindow);
    gui.show();
    gui.raise();
    gui.activateWindow();
}

void App::restoreOverlayAfterSettings()
{
    if (!m_overlayInitialized)
    {
        return;
    }

    // Re-apply the dormant overlay shell styles. Avoid show()/hide() mismatches
    // with Qt by only using showNoActivate after styles are restored.
    gui.hideSettingsPanel();
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
    for (Menu* m : loadedMenus) {
        if (m->triggerMod == 0 && m->triggerVk == 0) continue;
        Platform::InputBind oldBind;
        oldBind.mod = m->triggerMod;
        oldBind.input = m->triggerVk;
        m_inputRcvr.unregisterInputBinding(oldBind);
    }

    m_appConfig = appConfig;

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

    for (Menu* m : loadedMenus) {
        if (m->triggerMod == 0 && m->triggerVk == 0) continue;
        Platform::InputBind newBind;
        newBind.mod = m->triggerMod;
        newBind.input = m->triggerVk;
        m_inputRcvr.registerInputBinding(newBind);
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
        gui.setMenu(*activeMenu, getActionSlotVisualsForMenu(*activeMenu));
    }
}

QString App::getConfigPath() const
{
    return m_configPath;
}
