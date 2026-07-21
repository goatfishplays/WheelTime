/**
 * @file App.cpp
 * @brief Application singleton implementation.
 */

#include "App/App.hpp"

#include "App/ActionItems.hpp"
#include "App/MenuConfigLoader.hpp"
#include "App/SettingsWindow.hpp"

#include <Platform/Execute.hpp>
#include <Platform/Inputs.hpp>
#include <Platform/Window.hpp>

#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QThread>
#include <QTimer>

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
    for (const auto &item : action.items())
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
        if (Platform::InputReceiver::isHotkeyMessage(message, hotkeyId))
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
    : m_activeMenu(nullptr),
      m_gui(0),
      m_priorMousePos{},
      m_priorWindow{},
      m_executor(),
      m_inputReceiver(),
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
    if (!MenuConfigLoader::loadConfig(m_configPath, m_appConfig, m_actionLibrary, m_loadedMenus))
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<AI_Close>());
        m_actionLibrary.push_back(Action(std::move(items), "Config missing", "", "action-config-missing"));
        m_loadedMenus.push_back(std::make_unique<Menu>(
            0, 0, false, false, true, false, "Config Error",
            std::vector<std::string>{"action-config-missing"}, "menu-config-error"));
    }

    bool anyHotkey = false;
    for (const auto &menuPtr : m_loadedMenus)
    {
        Menu *m = menuPtr.get();
        if (m == nullptr || (m->triggerMod() == 0 && m->triggerVk() == 0))
        {
            continue;
        }
        anyHotkey = true;
        Platform::InputBinding bind;
        bind.mod = m->triggerMod();
        bind.input = m->triggerVk();
        m_inputReceiver.registerInputBinding(bind);
    }

    // Install native event filter to capture WM_HOTKEY
    m_hotkeyFilter = new HotkeyFilter(this);
    qApp->installNativeEventFilter(m_hotkeyFilter);

    // Connect escapePressed signal from Gui to hide and return focus
    QObject::connect(&m_gui, &Gui::escapePressed, [this]()
                     {
                         // Settings has an explicit Close button. Escaping out via
                         // hideGui() skips resumeHotkeys() / scheduler resume and
                         // leaves menu triggers permanently unregistered.
                         if (m_gui.isSettingsVisible())
                         {
                             return;
                         }
                         if (m_gui.isSearchVisible())
                         {
                             hideSearchOverlay();
                             return;
                         }
                         hideGui();
                     });

    // Move the old loadConfig block up above so it is loaded before m_inputReceiver registration

    m_actionHistory.load(historyPath());
    pruneActionHistory();

    if (!m_loadedMenus.empty())
    {
        m_activeMenu = m_loadedMenus.front().get();
        m_gui.setMenu(*m_activeMenu, actionSlotVisualsForMenu(*m_activeMenu));
    }

    initializeOverlay();

    // Warm the search palette's program index in the background so the first
    // palette open does not block on the initial shortcut-folder scan.
    m_gui.preloadSearchIndex();

    m_releaseWatchTimer = new QTimer(qApp);
    m_releaseWatchTimer->setInterval(16);
    QObject::connect(m_releaseWatchTimer, &QTimer::timeout, [this]()
                     { onExecuteOnReleaseTick(); });

    // The wheel overlay is intentionally non-activating / NoFocus so games keep
    // keyboard focus. Qt never delivers Escape to Gui::keyPressEvent in that
    // mode, so poll the physical key the same way execute-on-release does.
    m_escapeWatchTimer = new QTimer(qApp);
    m_escapeWatchTimer->setInterval(16);
    QObject::connect(m_escapeWatchTimer, &QTimer::timeout, [this]()
                     { onEscapeWatchTick(); });

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

    // Release OS hotkeys before clearing owned menus.
    if (!m_hotkeysSuspended)
    {
        for (const auto &menuPtr : m_loadedMenus)
        {
            Menu *m = menuPtr.get();
            if (m == nullptr || (m->triggerMod() == 0 && m->triggerVk() == 0))
            {
                continue;
            }
            Platform::InputBinding bind;
            bind.mod = m->triggerMod();
            bind.input = m->triggerVk();
            m_inputReceiver.unregisterInputBinding(bind);
        }
    }
    clearMenus();
}

void App::clearMenus()
{
    m_loadedMenus.clear();
}

void App::suspendHotkeys()
{
    if (m_hotkeysSuspended)
    {
        return;
    }

    // RegisterHotKey consumes matching KeyPresses, so the settings recorder
    // cannot capture a chord that is still bound. Drop live binds while editing.
    for (const auto &menuPtr : m_loadedMenus)
    {
        Menu *m = menuPtr.get();
        if (m == nullptr || (m->triggerMod() == 0 && m->triggerVk() == 0))
        {
            continue;
        }
        Platform::InputBinding bind;
        bind.mod = m->triggerMod();
        bind.input = m->triggerVk();
        m_inputReceiver.unregisterInputBinding(bind);
    }
    m_hotkeysSuspended = true;
}

void App::resumeHotkeys()
{
    if (!m_hotkeysSuspended)
    {
        return;
    }

    for (const auto &menuPtr : m_loadedMenus)
    {
        Menu *m = menuPtr.get();
        if (m == nullptr || (m->triggerMod() == 0 && m->triggerVk() == 0))
        {
            continue;
        }
        Platform::InputBinding bind;
        bind.mod = m->triggerMod();
        bind.input = m->triggerVk();
        m_inputReceiver.registerInputBinding(bind);
    }
    m_hotkeysSuspended = false;
}

void App::onHotkeyTriggered(int hotkeyId)
{
    // Don't fight the settings editor while the overlay is in focusable editor mode.
    if (m_gui.isSettingsVisible())
    {
        return;
    }

    // A menu hotkey while the search palette is open exits search into that
    // menu. Restore prior focus first so wheel mode keeps its invariant that
    // the underlying app owns the keyboard.
    if (m_gui.isSearchVisible())
    {
        hideSearchOverlay();
    }

    int mod = (hotkeyId >> 16) & 0xFFFF;
    int vk = hotkeyId & 0xFFFF;
    Menu *targetMenu = nullptr;
    for (const auto &menuPtr : m_loadedMenus)
    {
        Menu *m = menuPtr.get();
        if (m != nullptr && m->triggerMod() == mod && m->triggerVk() == vk)
        {
            targetMenu = m;
            break;
        }
    }

    // Same-menu hotkey (or unmatched) toggles closed while the wheel is up.
    // A different menu's hotkey switches to that menu instead of dismissing.
    if (m_gui.isLauncherVisible())
    {
        if (targetMenu == nullptr || targetMenu == m_activeMenu)
        {
            hideGui();
            return;
        }
        disarmExecuteOnRelease();
    }

    Menu *menuToShow = targetMenu;
    if (menuToShow == nullptr && m_activeMenu == nullptr && !m_loadedMenus.empty())
    {
        menuToShow = m_loadedMenus.front().get();
    }
    else if (menuToShow == nullptr)
    {
        menuToShow = m_activeMenu;
    }

    if (menuToShow == nullptr)
    {
        return;
    }

    m_activeMenu = menuToShow;
    showGui(m_activeMenu);

    if (m_activeMenu->executeOnRelease())
    {
        armExecuteOnRelease(m_activeMenu->triggerMod(), m_activeMenu->triggerVk());
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

void App::armEscapeDismiss()
{
    // Seed with the current key state so a held Escape does not instantly close.
    m_escapeWasDown = m_inputReceiver.isVirtualKeyDown(0x1B); // VK_ESCAPE
    if (m_escapeWatchTimer != nullptr)
    {
        m_escapeWatchTimer->start();
    }
}

void App::disarmEscapeDismiss()
{
    m_escapeWasDown = false;
    if (m_escapeWatchTimer != nullptr)
    {
        m_escapeWatchTimer->stop();
    }
}

void App::onEscapeWatchTick()
{
    if (!m_gui.isLauncherVisible())
    {
        disarmEscapeDismiss();
        return;
    }

    const bool escapeDown = m_inputReceiver.isVirtualKeyDown(0x1B); // VK_ESCAPE
    if (escapeDown && !m_escapeWasDown)
    {
        m_escapeWasDown = true;
        hideGui();
        return;
    }
    m_escapeWasDown = escapeDown;
}

void App::onExecuteOnReleaseTick()
{
    if (!m_executeOnReleaseArmed)
    {
        return;
    }

    // Wait until primary key AND every launcher modifier are up. Otherwise
    // releasing only Tab on Ctrl+Shift+Tab would inject keys with Ctrl/Shift still held.
    if (!m_inputReceiver.isChordFullyReleased(m_releaseWatchMod, m_releaseWatchVk))
    {
        return;
    }

    disarmExecuteOnRelease();

    if (!m_gui.isLauncherVisible())
    {
        return;
    }

    m_gui.refreshSelectionFromCursor();
    const int selected = m_gui.selectedActionIndex();
    if (m_activeMenu != nullptr && selected >= 0 && selected < m_activeMenu->actionCount()
        && findActionById(m_activeMenu->actionId(selected)) != nullptr)
    {
        executeAction(selected);
        return;
    }

    // No valid selection — close so a hold-open does not stick after release.
    hideGui();
}

Menu *App::activeMenu()
{
    return m_activeMenu;
}

Menu *App::findMenuById(const std::string &menuId)
{
    for (const auto &menuPtr : m_loadedMenus)
    {
        Menu *menu = menuPtr.get();
        if (menu != nullptr && menu->id() == menuId)
        {
            return menu;
        }
    }
    return nullptr;
}

const Menu *App::findMenuById(const std::string &menuId) const
{
    for (const auto &menuPtr : m_loadedMenus)
    {
        const Menu *menu = menuPtr.get();
        if (menu != nullptr && menu->id() == menuId)
        {
            return menu;
        }
    }
    return nullptr;
}

Action *App::findActionById(const std::string &actionId)
{
    for (Action &action : m_actionLibrary)
    {
        if (action.id() == actionId)
        {
            return &action;
        }
    }
    return nullptr;
}

const Action *App::findActionById(const std::string &actionId) const
{
    for (const Action &action : m_actionLibrary)
    {
        if (action.id() == actionId)
        {
            return &action;
        }
    }
    return nullptr;
}

std::vector<ActionSlotVisual> App::actionSlotVisualsForMenu(const Menu &menu) const
{
    std::vector<ActionSlotVisual> visuals;
    visuals.reserve(menu.actionCount());

    const QFileInfo configInfo(m_configPath);
    const QDir configDir = configInfo.absoluteDir();

    for (const std::string &actionId : menu.actionIds())
    {
        const Action *action = findActionById(actionId);
        ActionSlotVisual visual;
        visual.label = action != nullptr ? action->name() : "Missing Action";
        if (action != nullptr && !action->iconFilepath().empty())
        {
            const QString iconPath = QString::fromStdString(action->iconFilepath());
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

std::vector<Menu> App::menuCopies() const
{
    std::vector<Menu> menus;
    menus.reserve(m_loadedMenus.size());
    for (const auto &menuPtr : m_loadedMenus)
    {
        if (menuPtr)
        {
            menus.push_back(*menuPtr);
        }
    }
    return menus;
}

void App::gatherPriors()
{
    m_priorMousePos = m_inputReceiver.absoluteMousePosition();
    m_priorWindow.captureActiveWindow();
}

void App::restorePriors()
{
    // The non-activating overlay should leave keyboard focus where it already
    // is, so we intentionally avoid restoring focus here.
    if (m_activeMenu != nullptr && m_activeMenu->restoreMouseOnClose())
    {
        m_inputReceiver.setAbsoluteMousePosition(m_priorMousePos);
    }
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
    const Platform::Vec2 cursorPos = m_inputReceiver.absoluteMousePosition();
    Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
    const Platform::WindowRect bounds = overlayWindow.monitorBoundsForPoint(cursorPos.x, cursorPos.y);
    m_gui.setGeometry(bounds.x, bounds.y, bounds.width, bounds.height);
    m_gui.show();
    overlayWindow.setTransparentOverlay(true);
    overlayWindow.setNonActivating(true);
    overlayWindow.setTopmost(true);
    overlayWindow.setBounds(bounds);
    overlayWindow.showNoActivate();
    m_gui.enterDormantOverlay();
    overlayWindow.setClickThrough(true);
    m_overlayInitialized = true;
}

void App::configureOverlayForCursor()
{
    const Platform::Vec2 cursorPos = m_inputReceiver.absoluteMousePosition();
    Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
    const Platform::WindowRect bounds = overlayWindow.monitorBoundsForPoint(cursorPos.x, cursorPos.y);
    // Keep Qt's widget geometry in sync with the native overlay bounds. The
    // shell widget owns the centered panel layout, so it must know its actual
    // monitor-sized rect for rendering and hit-testing to behave correctly.
    m_gui.setGeometry(bounds.x, bounds.y, bounds.width, bounds.height);
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
        const std::string menuId = menu->id();
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
    if (m_gui.isSearchVisible())
    {
        hideSearchOverlay();
    }

    initializeOverlay();
    gatherPriors();
    m_activeMenu = menu;
    // Size the overlay to the current monitor first so radius/layout use final dims.
    configureOverlayForCursor();

    if (m_activeMenu->centerMouseOnOpen())
    {
        const Platform::Vec2 cursorPos = m_inputReceiver.absoluteMousePosition();
        Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
        const Platform::WindowRect bounds =
            overlayWindow.monitorBoundsForPoint(cursorPos.x, cursorPos.y);
        // Start the cursor slightly above the wheel center; scales with the
        // monitor so the nudge feels the same across resolutions.
        const int upwardNudge = bounds.height * 2 / 100;
        m_inputReceiver.setAbsoluteMousePosition(Platform::Vec2{
            bounds.x + bounds.width / 2,
            bounds.y + bounds.height / 2 - upwardNudge});
    }

    m_gui.setMenu(*m_activeMenu, actionSlotVisualsForMenu(*m_activeMenu));

    Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
    overlayWindow.setClickThrough(false);
    overlayWindow.showNoActivate();
    m_gui.enterInteractiveOverlay();
    armEscapeDismiss();
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

    // Settings must leave through the same cleanup as Close (resume OS hotkeys
    // and the scheduler). The wheel dismiss path below does not do that.
    if (m_gui.isSettingsVisible())
    {
        if (m_scheduler)
        {
            m_scheduler->resume();
        }
        restoreOverlayAfterSettings();
        return;
    }

    disarmExecuteOnRelease();
    disarmEscapeDismiss();
    initializeOverlay();
    m_gui.enterDormantOverlay();
    Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
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
    if (m_gui.isSettingsVisible())
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
    Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
    overlayWindow.setNonActivating(false);
    overlayWindow.setClickThrough(false);
    overlayWindow.setTopmost(true);

    // showSearchPanel hides the wheel/settings panels and flips the mode enum.
    m_gui.showSearchPanel(config);
    m_gui.show();
    m_gui.raise();
    m_gui.activateWindow();
    // Beat the Windows foreground lock if activateWindow was not enough.
    overlayWindow.focus();
    m_gui.focusSearchInput();
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

    if (!m_gui.isSearchVisible())
    {
        return;
    }

    m_gui.hideSearchPanel();

    // Back to the dormant shell: non-activating, click-through, topmost.
    Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
    overlayWindow.setTransparentOverlay(true);
    overlayWindow.setNonActivating(true);
    overlayWindow.setClickThrough(true);
    overlayWindow.setTopmost(true);
    overlayWindow.showNoActivate();

    // Search genuinely steals focus (unlike the wheel), so hand it back.
    if (restoreFocus)
    {
        m_priorWindow.focus();
    }
}

void App::executeAction(int actionIndex)
{
    // Click-to-execute while holding must not also fire on release.
    disarmExecuteOnRelease();

    if (m_activeMenu == nullptr || actionIndex < 0 || actionIndex >= m_activeMenu->actionCount())
    {
        return;
    }

    Action *action = findActionById(m_activeMenu->actionId(actionIndex));
    if (action == nullptr)
    {
        return;
    }

    // Clear leftover launcher modifiers before inject (click-while-holding path).
    // Release-execute also waits for a full physical chord-up; this is the safety net.
    if (m_activeMenu->triggerMod() != 0)
    {
        m_executor.modifiersUp(m_activeMenu->triggerMod());
    }

    executeActionById(action->id());

    if (m_activeMenu->exitOnAction() && m_gui.isLauncherVisible())
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
        m_actionHistory.recordUse(action->id());
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
    ids.reserve(m_actionLibrary.size());
    for (const Action &action : m_actionLibrary)
    {
        if (!action.id().empty())
        {
            ids.push_back(action.id());
        }
    }
    m_actionHistory.pruneToLibrary(ids);
}

void App::saveActionHistory()
{
    m_actionHistory.save(historyPath());
}

void App::resetActionFrequencies()
{
    m_actionHistory.clearFrequencies();
    saveActionHistory();
}

Scheduler &App::scheduler() noexcept
{
    return *m_scheduler;
}

const Scheduler &App::scheduler() const noexcept
{
    return *m_scheduler;
}

std::vector<std::unique_ptr<Menu>> &App::loadedMenus() noexcept
{
    return m_loadedMenus;
}

const std::vector<std::unique_ptr<Menu>> &App::loadedMenus() const noexcept
{
    return m_loadedMenus;
}

std::vector<Action> &App::actionLibrary() noexcept
{
    return m_actionLibrary;
}

const std::vector<Action> &App::actionLibrary() const noexcept
{
    return m_actionLibrary;
}

Platform::Executor &App::executor() noexcept
{
    return m_executor;
}

const Platform::Executor &App::executor() const noexcept
{
    return m_executor;
}

void App::showSettingsWindow()
{
    // Settings replaces the search palette; settings takes focus itself, so
    // there is no point bouncing focus back to the prior window first.
    if (m_gui.isSearchVisible())
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
    Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
    overlayWindow.setTransparentOverlay(true);
    overlayWindow.setNonActivating(false);
    overlayWindow.setClickThrough(false);
    overlayWindow.setTopmost(true);

    // Pause macros while editing; resume when the window closes.
    m_scheduler->pause();
    // Drop OS hotkeys so the recorder can capture chords that are already bound.
    suspendHotkeys();
    m_settingsWindow->loadWorkingCopy(m_appConfig, m_actionLibrary, menuCopies());
    m_gui.showSettingsPanel(m_settingsWindow);
    m_gui.show();
    m_gui.raise();
    m_gui.activateWindow();
}

void App::restoreOverlayAfterSettings()
{
    if (!m_overlayInitialized)
    {
        return;
    }

    // Re-apply the dormant overlay shell styles. Avoid show()/hide() mismatches
    // with Qt by only using showNoActivate after styles are restored.
    m_gui.hideSettingsPanel();
    m_gui.enterDormantOverlay();
    resumeHotkeys();
    Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
    overlayWindow.setTransparentOverlay(true);
    overlayWindow.setNonActivating(true);
    overlayWindow.setClickThrough(true);
    overlayWindow.setTopmost(true);
    overlayWindow.showNoActivate();
}

bool App::saveConfig()
{
    return MenuConfigLoader::saveConfig(m_configPath, m_appConfig, m_actionLibrary, m_loadedMenus);
}

bool App::applyConfig(const AppConfig &appConfig, const std::vector<Action> &actions, const std::vector<Menu> &menus)
{
    // Skip unregister while settings has hotkeys suspended; resumeHotkeys will
    // bind whatever ends up in m_loadedMenus when the editor closes.
    if (!m_hotkeysSuspended)
    {
        for (const auto &menuPtr : m_loadedMenus)
        {
            Menu *m = menuPtr.get();
            if (m == nullptr || (m->triggerMod() == 0 && m->triggerVk() == 0))
            {
                continue;
            }
            Platform::InputBinding oldBind;
            oldBind.mod = m->triggerMod();
            oldBind.input = m->triggerVk();
            m_inputReceiver.unregisterInputBinding(oldBind);
        }
    }

    m_appConfig = appConfig;

    // Drop in-flight macros that may reference old library Actions / menus.
    if (m_scheduler)
    {
        m_scheduler->cancelAll();
    }
    const std::string previousActiveMenuId = m_activeMenu == nullptr ? "" : m_activeMenu->id();

    // Swap the full runtime model in one step so menus and the shared action
    // library stay in sync after settings edits or schema migration.
    m_actionLibrary = actions;
    clearMenus();
    m_loadedMenus.reserve(menus.size());
    for (const Menu &menu : menus)
    {
        m_loadedMenus.push_back(std::make_unique<Menu>(menu));
    }

    if (!m_hotkeysSuspended)
    {
        for (const auto &menuPtr : m_loadedMenus)
        {
            Menu *m = menuPtr.get();
            if (m == nullptr || (m->triggerMod() == 0 && m->triggerVk() == 0))
            {
                continue;
            }
            Platform::InputBinding newBind;
            newBind.mod = m->triggerMod();
            newBind.input = m->triggerVk();
            m_inputReceiver.registerInputBinding(newBind);
        }
    }

    m_activeMenu = previousActiveMenuId.empty() ? nullptr : findMenuById(previousActiveMenuId);
    if (m_activeMenu == nullptr && !m_loadedMenus.empty())
    {
        m_activeMenu = m_loadedMenus.front().get();
    }

    const bool saved = saveConfig();
    pruneActionHistory();
    saveActionHistory();
    refreshActiveMenu();
    return saved;
}

void App::refreshActiveMenu()
{
    if (m_activeMenu != nullptr)
    {
        m_gui.setMenu(*m_activeMenu, actionSlotVisualsForMenu(*m_activeMenu));
    }
}

QString App::configPath() const
{
    return m_configPath;
}
