/**
 * @file App.cpp
 * @brief Application singleton implementation.
 */

#include "App/App.hpp"

#include "App/ActionItems.hpp"
#include "App/Log.hpp"
#include "App/MenuConfigLoader.hpp"
#include "App/SettingsWindow.hpp"
#include "App/Theme.hpp"

#include <Platform/Execute.hpp>
#include <Platform/Inputs.hpp>
#include <Platform/Window.hpp>

#include <QAbstractNativeEventFilter>
#include <QAction>
#include <QApplication>
#include <QCursor>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QMenu>
#include <QMetaObject>
#include <QPoint>
#include <QRect>
#include <QScreen>
#include <QString>
#include <QSystemTrayIcon>
#include <QThread>
#include <QTimer>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace Application;

namespace
{
Platform::InputBinding bindingForMenu(const Menu &menu)
{
    Platform::InputBinding bind;
    bind.mod = menu.triggerMod();
    bind.input = menu.triggerVk();
    bind.useLowLevelHook = menu.useLowLevelHook();
    return bind;
}

void deliverLlHotkey(int hotkeyId, void *userData)
{
    auto *app = static_cast<App *>(userData);
    // Queue onto the GUI loop — this may run inside WH_KEYBOARD_LL.
    QMetaObject::invokeMethod(
        qApp,
        [app, hotkeyId]()
        {
            if (app != nullptr)
            {
                app->onHotkeyTriggered(hotkeyId);
            }
        },
        Qt::QueuedConnection);
}

void deliverOverlayMouseButton(Platform::OverlayMouseButton button, bool pressed, void *userData)
{
    auto *app = static_cast<App *>(userData);
    QMetaObject::invokeMethod(
        qApp,
        [app, button, pressed]()
        {
            if (app != nullptr)
            {
                app->onOverlayMouseButton(button, pressed);
            }
        },
        Qt::QueuedConnection);
}

[[nodiscard]] QPoint clampPointToRect(QPoint pos, const QRect &bounds)
{
    pos.setX(std::clamp(pos.x(), bounds.left(), bounds.right()));
    pos.setY(std::clamp(pos.y(), bounds.top(), bounds.bottom()));
    return pos;
}

[[nodiscard]] QScreen *screenAtQtCursor()
{
    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (screen == nullptr)
    {
        screen = QGuiApplication::primaryScreen();
    }
    return screen;
}

/// @brief Qt widget geometry in device-independent pixels for the cursor's screen.
void applyQtOverlayGeometry(QWidget &gui)
{
    if (QScreen *screen = screenAtQtCursor())
    {
        gui.setGeometry(screen->geometry());
    }
}
} // namespace

void App::applyTheme()
{
    static bool applied = false;
    static bool lastDark = false;
    static QString lastOverlayPath;
    static qint64 lastOverlayMtime = -1;

    const QString overlayPath = Theme::resolveOverlayPath(m_appConfig.themeOverlayPath, m_configPath);
    qint64 overlayMtime = -1;
    if (!overlayPath.isEmpty())
    {
        const QFileInfo overlayInfo(overlayPath);
        if (overlayInfo.exists())
        {
            overlayMtime = overlayInfo.lastModified().toMSecsSinceEpoch();
        }
    }

    if (applied && lastDark == m_appConfig.darkMode && lastOverlayPath == overlayPath
        && lastOverlayMtime == overlayMtime)
    {
        return;
    }

    qApp->setStyleSheet(Theme::composeStylesheet(m_appConfig.darkMode, overlayPath));
    applied = true;
    lastDark = m_appConfig.darkMode;
    lastOverlayPath = overlayPath;
    lastOverlayMtime = overlayMtime;
}

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
    explicit HotkeyFilter(App *app, Platform::InputReceiver *inputs)
        : m_app(app)
        , m_inputs(inputs)
    {
    }

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override
    {
        (void)eventType;
        (void)result;
        if (m_inputs != nullptr)
        {
            m_inputs->processNativeMessage(message);
        }
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
    Platform::InputReceiver *m_inputs;
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
            0, 0, false, false, true, false, false, "Config Error",
            std::vector<std::string>{"action-config-missing"}, "menu-config-error"));
    }

    applyTheme();
    Theme::seedExampleOverlay(m_configPath);

    m_inputReceiver.setHotkeyTriggeredHandler(&deliverLlHotkey, this);

    bool anyHotkey = false;
    for (const auto &menuPtr : m_loadedMenus)
    {
        Menu *m = menuPtr.get();
        if (m == nullptr || (m->triggerMod() == 0 && m->triggerVk() == 0))
        {
            continue;
        }
        anyHotkey = true;
        m_inputReceiver.registerInputBinding(bindingForMenu(*m));
    }

    // Install native event filter to capture WM_HOTKEY
    m_hotkeyFilter = new HotkeyFilter(this, &m_inputReceiver);
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
        applyMenuToGui(*m_activeMenu);
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

    m_mouseCaptureTimer = new QTimer(qApp);
    m_mouseCaptureTimer->setInterval(8);
    QObject::connect(m_mouseCaptureTimer, &QTimer::timeout, [this]()
                     { onGameMouseCaptureTick(); });

    setupTrayIcon();

    QTimer::singleShot(0, [this]() { showSettingsWindow(); });
}

App::~App()
{
    endGameMouseCaptureSession();
    if (m_trayIcon != nullptr)
    {
        m_trayIcon->hide();
    }

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

    m_inputReceiver.setHotkeyTriggeredHandler(nullptr, nullptr);

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
            m_inputReceiver.unregisterInputBinding(bindingForMenu(*m));
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

    // RegisterHotKey / LL hooks consume matching KeyPresses, so the settings
    // recorder cannot capture a chord that is still bound. Drop live binds while editing.
    for (const auto &menuPtr : m_loadedMenus)
    {
        Menu *m = menuPtr.get();
        if (m == nullptr || (m->triggerMod() == 0 && m->triggerVk() == 0))
        {
            continue;
        }
        m_inputReceiver.unregisterInputBinding(bindingForMenu(*m));
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
        m_inputReceiver.registerInputBinding(bindingForMenu(*m));
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

void App::beginGameMouseCaptureSession()
{
    endGameMouseCaptureSession();
    if (m_appConfig.gameMouseCapture == GameMouseCaptureMode::Off)
    {
        return;
    }

    const QPoint qtCursor = QCursor::pos();
    m_virtualCursorPos = Platform::Vec2{qtCursor.x(), qtCursor.y()};
    m_lastOsCursorPos = m_inputReceiver.absoluteMousePosition();
    m_usingVirtualCursor = true;
    m_stoleGameFocus = false;
    m_overlayWarpHits = 0;
    m_ignoreWarpUntilMs = QDateTime::currentMSecsSinceEpoch() + 150;
    m_lastStealAttemptMs = 0;
    m_gui.setVirtualCursor(qtCursor);
    m_inputReceiver.releaseCursorClip();
    m_inputReceiver.beginOverlayMouseSession(
        reinterpret_cast<void *>(m_gui.winId()), &deliverOverlayMouseButton, this);
    if (m_mouseCaptureTimer != nullptr)
    {
        m_mouseCaptureTimer->start();
    }
}

void App::endGameMouseCaptureSession()
{
    if (m_mouseCaptureTimer != nullptr)
    {
        m_mouseCaptureTimer->stop();
    }
    m_inputReceiver.endOverlayMouseSession();
    m_usingVirtualCursor = false;
    m_overlayWarpHits = 0;
    m_gui.setVirtualCursor(std::nullopt);
    if (m_overlayInitialized)
    {
        m_gui.setAttribute(Qt::WA_ShowWithoutActivating, true);
        m_gui.setFocusPolicy(Qt::NoFocus);
        Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
        overlayWindow.setNonActivating(true);
    }
}

void App::onGameMouseCaptureTick()
{
    if (!m_gui.isLauncherVisible())
    {
        endGameMouseCaptureSession();
        return;
    }

    m_inputReceiver.releaseCursorClip();

    int dx = 0;
    int dy = 0;
    m_inputReceiver.pollOverlayMouseDelta(dx, dy);
    const int deviceMove = std::abs(dx) + std::abs(dy);
    const Platform::Vec2 osCursor = m_inputReceiver.absoluteMousePosition();
    const int osMove = std::abs(osCursor.x - m_lastOsCursorPos.x) + std::abs(osCursor.y - m_lastOsCursorPos.y);
    m_lastOsCursorPos = osCursor;

    if (m_usingVirtualCursor && (dx != 0 || dy != 0))
    {
        const qreal dpr = std::max(m_gui.devicePixelRatioF(), qreal(0.01));
        const QRect bounds = m_gui.geometry();
        const QPoint next = clampPointToRect(
            QPoint(m_virtualCursorPos.x + qRound(static_cast<qreal>(dx) / dpr),
                   m_virtualCursorPos.y + qRound(static_cast<qreal>(dy) / dpr)),
            bounds);
        m_virtualCursorPos = Platform::Vec2{next.x(), next.y()};
        m_gui.setVirtualCursor(next);
    }

    if (m_appConfig.gameMouseCapture != GameMouseCaptureMode::StealIfLocked)
    {
        return;
    }

    Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
    if (m_stoleGameFocus)
    {
        if (overlayWindow.isForeground() && !m_inputReceiver.isCursorClipActive())
        {
            return;
        }
        // Game took the cursor back; resume the virtual cursor.
        m_stoleGameFocus = false;
        m_usingVirtualCursor = true;
        m_gui.setVirtualCursor(QPoint(m_virtualCursorPos.x, m_virtualCursorPos.y));
    }

    m_overlayWarpHits += m_inputReceiver.takeOverlayMouseWarpCount();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now < m_ignoreWarpUntilMs)
    {
        m_overlayWarpHits = 0;
    }

    // Minecraft recenters with small non-injected warps: the device moves but
    // GetCursorPos stays near the game window center.
    const bool rubberBandLock = deviceMove >= 6 && osMove <= 2;
    const bool clipped = m_inputReceiver.isCursorClipActive();
    if (clipped || m_overlayWarpHits >= 2 || rubberBandLock)
    {
        stealOverlayFocus();
    }
}

void App::stealOverlayFocus()
{
    if (!m_gui.isLauncherVisible())
    {
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastStealAttemptMs < 200)
    {
        return;
    }
    m_lastStealAttemptMs = now;

    m_gui.setAttribute(Qt::WA_ShowWithoutActivating, false);
    m_gui.setFocusPolicy(Qt::ClickFocus);

    Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
    overlayWindow.setNonActivating(false);
    overlayWindow.setClickThrough(false);
    overlayWindow.setTopmost(true);
    m_gui.activateWindow();
    overlayWindow.focus();

    if (!overlayWindow.isForeground())
    {
        std::cerr << "[mouse-capture] steal focus failed; keeping virtual cursor\n";
        return;
    }

    m_stoleGameFocus = true;
    QCursor::setPos(QPoint(m_virtualCursorPos.x, m_virtualCursorPos.y));
    m_lastOsCursorPos = m_inputReceiver.absoluteMousePosition();
    m_ignoreWarpUntilMs = now + 150;
    m_usingVirtualCursor = false;
    m_gui.setVirtualCursor(std::nullopt);
    m_gui.refreshSelectionFromCursor();
    std::cerr << "[mouse-capture] stole foreground from game\n";
}

void App::onOverlayMouseButton(Platform::OverlayMouseButton button, bool pressed)
{
    if (!pressed || !m_gui.isLauncherVisible())
    {
        return;
    }

    if (button == Platform::OverlayMouseButton::Right)
    {
        hideGui();
        return;
    }

    if (button != Platform::OverlayMouseButton::Left)
    {
        return;
    }

    if (virtualCursorHitsSettingsButton())
    {
        showSettingsWindow();
        return;
    }

    executeAction(m_gui.selectedActionIndex());
}

bool App::virtualCursorHitsSettingsButton() const
{
    return m_gui.hitsSettingsButton(QPoint(m_virtualCursorPos.x, m_virtualCursorPos.y));
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
    if (m_stoleGameFocus)
    {
        m_priorWindow.focus();
        m_stoleGameFocus = false;
    }
    // The non-activating overlay should leave keyboard focus where it already
    // is, so we intentionally avoid restoring focus here unless we stole it.
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

    // Give Qt DIP geometry for the cursor's screen, then size the HWND in
    // native pixels. Re-apply Qt geometry after SetWindowPos so widget
    // coordinates stay device-independent on scaled monitors.
    Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
    applyQtOverlayGeometry(m_gui);
    const Platform::Vec2 cursorPos = m_inputReceiver.absoluteMousePosition();
    const Platform::WindowRect bounds = overlayWindow.monitorBoundsForPoint(cursorPos.x, cursorPos.y);
    m_gui.show();
    overlayWindow.setTransparentOverlay(true);
    overlayWindow.setNonActivating(true);
    overlayWindow.setTopmost(true);
    overlayWindow.setBounds(bounds);
    applyQtOverlayGeometry(m_gui);
    overlayWindow.showNoActivate();
    m_gui.enterDormantOverlay();
    overlayWindow.setClickThrough(true);
    m_overlayInitialized = true;
}

void App::configureOverlayForCursor()
{
    Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
    // Qt geometry is device-independent pixels; Win32 monitor rects are native
    // pixels. Using the native rect for QWidget::setGeometry makes mapToGlobal
    // and SetCursorPos disagree on scaled or mixed-DPI monitors, so the drawn
    // cursor sits on the wheel while the OS cursor does not.
    const Platform::Vec2 cursorPos = m_inputReceiver.absoluteMousePosition();
    const Platform::WindowRect bounds = overlayWindow.monitorBoundsForPoint(cursorPos.x, cursorPos.y);
    overlayWindow.setBounds(bounds);
    applyQtOverlayGeometry(m_gui);
    overlayWindow.setTransparentOverlay(true);
    overlayWindow.setNonActivating(true);
    overlayWindow.setTopmost(true);
}

void App::applyMenuToGui(const Menu &menu)
{
    m_gui.applyRice(Theme::resolve(m_appConfig.rice, menu.rice()));
    m_gui.setMenu(menu, actionSlotVisualsForMenu(menu));
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
    const bool alreadyOpen = m_gui.isLauncherVisible();
    if (!alreadyOpen)
    {
        gatherPriors();
    }
    m_activeMenu = menu;
    // Stale picks should not survive a fresh open if hide somehow skipped flush.
    m_deferredActionIds.clear();
    // Size the overlay to the current monitor first so radius/layout use final dims.
    configureOverlayForCursor();
    applyMenuToGui(*m_activeMenu);

    Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
    overlayWindow.setClickThrough(false);
    overlayWindow.showNoActivate();
    m_gui.enterInteractiveOverlay();
    if (m_gui.layout() != nullptr)
    {
        m_gui.layout()->activate();
    }

    if (m_activeMenu->centerMouseOnOpen())
    {
        const RiceSettings rice = Theme::resolve(m_appConfig.rice, m_activeMenu->rice());
        const QPoint target = m_gui.mouseOpenGlobalPosition(
            rice.mouseOpenOffsetXFraction, rice.mouseOpenOffsetYFraction);
        // QCursor::setPos converts Qt DIP to native pixels; SetCursorPos does not.
        QCursor::setPos(target);
        m_gui.refreshSelectionFromCursor();
    }

    beginGameMouseCaptureSession();
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
    endGameMouseCaptureSession();
    initializeOverlay();
    m_gui.enterDormantOverlay();
    Platform::Window overlayWindow(reinterpret_cast<void *>(m_gui.winId()));
    overlayWindow.setClickThrough(true);
    overlayWindow.showNoActivate();
    restorePriors();
    flushDeferredActions();
}

void App::flushDeferredActions()
{
    if (m_deferredActionIds.empty())
    {
        return;
    }

    std::vector<std::string> ids;
    ids.swap(m_deferredActionIds);
    for (const std::string &actionId : ids)
    {
        executeActionById(actionId);
    }
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
    if (m_gui.isLauncherVisible())
    {
        disarmEscapeDismiss();
        endGameMouseCaptureSession();
        m_stoleGameFocus = false;
    }
    initializeOverlay();

    // Keep the wheel's captured foreground window when search opens over it so
    // we do not snapshot WheelTime itself after StealIfLocked took activation.
    if (!m_gui.isLauncherVisible())
    {
        gatherPriors();
    }
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

    if (m_activeMenu->deferUntilExit() && m_gui.isLauncherVisible())
    {
        m_deferredActionIds.push_back(action->id());
    }
    else
    {
        executeActionById(action->id());
    }

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

void App::setupTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        qWarning("WheelTime: system tray is unavailable; tray icon will not be shown.");
        return;
    }

    const QIcon appIcon(QStringLiteral(":/icons/wheelTime.png"));
    if (!appIcon.isNull())
    {
        qApp->setWindowIcon(appIcon);
    }

    // Parent to the long-lived Gui shell so lifetime matches App, not QApplication.
    m_trayMenu = new QMenu(&m_gui);
    QAction *openSettingsAction = m_trayMenu->addAction(QStringLiteral("Open Settings"));
    QAction *openLogAction = m_trayMenu->addAction(QStringLiteral("Open Log"));
    QAction *searchActionsAction = m_trayMenu->addAction(QStringLiteral("Search Actions"));
    QAction *searchMenusAction = m_trayMenu->addAction(QStringLiteral("Search Menus"));
    m_trayMenu->addSeparator();
    QAction *exitAction = m_trayMenu->addAction(QStringLiteral("Exit"));

    QObject::connect(openSettingsAction, &QAction::triggered, [this]()
                     { showSettingsWindow(); });
    QObject::connect(openLogAction, &QAction::triggered, []()
                     { Log::instance().showWindow(); });
    QObject::connect(searchActionsAction, &QAction::triggered, [this]()
                     {
                         SearchConfig config;
                         config.searchActions = true;
                         config.searchPrograms = false;
                         config.searchMenus = false;
                         config.webSearch = false;
                         openSearchFromTray(config);
                     });
    QObject::connect(searchMenusAction, &QAction::triggered, [this]()
                     {
                         SearchConfig config;
                         config.searchActions = false;
                         config.searchPrograms = false;
                         config.searchMenus = true;
                         config.webSearch = false;
                         openSearchFromTray(config);
                     });
    QObject::connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon = new QSystemTrayIcon(appIcon, &m_gui);
    m_trayIcon->setToolTip(QStringLiteral("WheelTime"));
    m_trayIcon->setContextMenu(m_trayMenu);
    QObject::connect(m_trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason)
                     {
                         if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
                         {
                             showSettingsWindow();
                         }
                     });
    m_trayIcon->show();
}

void App::openSearchFromTray(const SearchConfig &config)
{
    if (m_gui.isSettingsVisible())
    {
        if (m_scheduler)
        {
            m_scheduler->resume();
        }
        restoreOverlayAfterSettings();
    }

    showSearchOverlay(config);
}

void App::showSettingsWindow()
{
    // Settings replaces the search palette; settings takes focus itself, so
    // there is no point bouncing focus back to the prior window first.
    if (m_gui.isSearchVisible())
    {
        hideSearchOverlay(/*restoreFocus=*/false);
    }

    // Treat opening settings as leaving the wheel session: restore cursor and
    // flush any defer-until-exit picks so they are not stranded.
    if (m_gui.isLauncherVisible())
    {
        disarmExecuteOnRelease();
        disarmEscapeDismiss();
        endGameMouseCaptureSession();
        restorePriors();
        flushDeferredActions();
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

void App::beginTriggerCapture(Platform::ChordCaptureHandler handler, void *userData)
{
    m_inputReceiver.beginChordCapture(handler, userData);
}

void App::endTriggerCapture()
{
    m_inputReceiver.endChordCapture();
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
            m_inputReceiver.unregisterInputBinding(bindingForMenu(*m));
        }
    }

    m_appConfig = appConfig;
    applyTheme();

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
            m_inputReceiver.registerInputBinding(bindingForMenu(*m));
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
        applyMenuToGui(*m_activeMenu);
    }
}

QString App::configPath() const
{
    return m_configPath;
}

const AppConfig &App::appConfig() const noexcept
{
    return m_appConfig;
}
