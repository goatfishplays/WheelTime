/**
 * @file App.hpp
 * @brief Application singleton: menus, actions, input, and scheduler ownership.
 */
#pragma once

#include <Platform/Execute.hpp>
#include <Platform/Inputs.hpp>
#include <Platform/Window.hpp>

#include "App/Action.hpp"
#include "App/ActionHistory.hpp"
#include "App/Gui.hpp"
#include "App/Menu.hpp"
#include "App/Scheduler.hpp"
#include "App/SearchConfig.hpp"

#include <QApplication>
#include <QString>

#include <memory>
#include <vector>

class QTimer;

namespace Application
{
/// @brief Forward declaration for the non-modal settings editor window.
class SettingsWindow;

struct AppConfig
{
};

/**
 * @brief Owns the long-lived runtime state for WheelTime.
 *
 * `App` keeps two related collections:
 * - `actionLibrary()`: reusable actions keyed by stable IDs
 * - `loadedMenus()`: wheel menus that reference those actions by ID
 *
 * That split lets the same action be reused in multiple menus and keeps
 * the runtime aligned with the JSON config edited from the settings window.
 */
class App
{
public:
    App(const App &) = delete;
    App &operator=(const App &) = delete;
    App(App &&) = delete;
    App &operator=(App &&) = delete;

    static App &instance()
    {
        static App app;
        return app;
    }

    /// @brief Loaded wheel menus owned by App. Each menu owns only slot references.
    [[nodiscard]] std::vector<std::unique_ptr<Menu>> &loadedMenus() noexcept;
    [[nodiscard]] const std::vector<std::unique_ptr<Menu>> &loadedMenus() const noexcept;
    /// @brief Shared library of reusable actions referenced by menu slots.
    [[nodiscard]] std::vector<Action> &actionLibrary() noexcept;
    [[nodiscard]] const std::vector<Action> &actionLibrary() const noexcept;
    /// @brief Platform action executor (keystrokes, launches, sockets, …).
    [[nodiscard]] Platform::Executor &executor() noexcept;
    [[nodiscard]] const Platform::Executor &executor() const noexcept;

    /// @brief Returns the menu currently shown in the radial UI.
    [[nodiscard]] Menu *activeMenu();
    /// @brief Look up a loaded menu by its stable config ID.
    [[nodiscard]] Menu *findMenuById(const std::string &menuId);
    [[nodiscard]] const Menu *findMenuById(const std::string &menuId) const;
    /// @brief Look up a reusable action by its stable config ID.
    [[nodiscard]] Action *findActionById(const std::string &actionId);
    [[nodiscard]] const Action *findActionById(const std::string &actionId) const;
    /// @brief Resolve user-facing labels/icons for the action IDs in a menu.
    std::vector<ActionSlotVisual> actionSlotVisualsForMenu(const Menu &menu) const;
    /// @brief Build value copies of menus for the settings working copy.
    std::vector<Menu> menuCopies() const;

    /// @brief Shows the radial launcher for a specific menu.
    void showGui(Menu *menu);
    /// @brief Hides the radial launcher and restores prior focus/mouse state.
    void hideGui();
    /**
     * @brief Shows the search palette overlay with the given filter set.
     *
     * Unlike the wheel this takes real keyboard focus; the prior foreground
     * window is captured first and restored by hideSearchOverlay.
     */
    void showSearchOverlay(const SearchConfig &config);
    /**
     * @brief Hides the search palette and re-applies dormant overlay styles.
     * @param restoreFocus Give focus back to the previously active window.
     * Pass false when the user chose a new focus target themselves (alt-tab)
     * or when another WheelTime surface takes over (settings).
     */
    void hideSearchOverlay(bool restoreFocus = true);
    /// @brief Executes the action referenced by the selected slot index.
    void executeAction(int actionIndex);
    /// @brief Records usage and schedules a library action by stable ID.
    void executeActionById(const std::string &actionId);
    /// @brief True for history/cancel helper actions excluded from MRU/MFU.
    static bool isHistoryMetaAction(const Action &action);
    /**
     * @brief Library Action at 1-based recent rank @p n, or nullptr.
     *
     * Skips history/cancel helper Actions (nth-recent, nth-frequent, cancel)
     * so those can never resolve to themselves. Does not record a use.
     */
    Action *nthRecentAction(int n);
    /**
     * @brief Library Action at 1-based frequency rank @p n, or nullptr.
     *
     * Skips history/cancel helper Actions the same way as nthRecentAction.
     * Does not record a use.
     */
    Action *nthFrequentAction(int n);
    /// @brief Clears all action launch frequency counts and persists history.
    void resetActionFrequencies();
    /// @brief Global hotkey callback used to show or hide the launcher.
    void onHotkeyTriggered(int hotkeyId);
    /// @brief Opens the non-modal settings editor window.
    void showSettingsWindow();
    /// @brief Restores the dormant overlay shell after settings closes.
    void restoreOverlayAfterSettings();
    /// @brief Persists the current runtime config to disk.
    bool saveConfig();
    /// @brief Replaces the live runtime config with an edited working copy.
    bool applyConfig(const AppConfig &appConfig, const std::vector<Action> &actions, const std::vector<Menu> &menus);
    /// @brief Re-renders the active menu after config changes.
    void refreshActiveMenu();
    QString configPath() const;

    /// @brief The multithreaded Action scheduler used for all launcher macros.
    [[nodiscard]] Scheduler &scheduler() noexcept;
    [[nodiscard]] const Scheduler &scheduler() const noexcept;

private:
    Menu *m_activeMenu;
    Platform::Vec2 m_priorMousePos;
    Platform::Window m_priorWindow;
    bool m_overlayInitialized{false};

    void gatherPriors();
    void restorePriors();
    void initializeOverlay();
    void configureOverlayForCursor();
    /// @brief Unregisters live menu hotkeys so settings can capture those keys.
    void suspendHotkeys();
    /// @brief Re-registers menu hotkeys after settings closes.
    void resumeHotkeys();
    /// @brief Clears the currently loaded menus (unique_ptr ownership).
    void clearMenus();
    /// @brief Starts polling for hotkey chord release (execute-on-release mode).
    void armExecuteOnRelease(int mod, int vk);
    /// @brief Stops release polling without executing.
    void disarmExecuteOnRelease();
    /// @brief QTimer callback: execute current selection once the chord is up.
    void onExecuteOnReleaseTick();
    /// @brief Starts Escape edge-polling while the non-activating wheel is up.
    void armEscapeDismiss();
    /// @brief Stops Escape polling when the wheel closes.
    void disarmEscapeDismiss();
    /// @brief QTimer callback: dismiss the wheel on Escape press edges.
    void onEscapeWatchTick();
    App();
    ~App();

    std::vector<std::unique_ptr<Menu>> m_loadedMenus;
    std::vector<Action> m_actionLibrary;
    Platform::Executor m_executor;
    Gui m_gui;

    Platform::InputReceiver m_inputReceiver;
    class QAbstractNativeEventFilter *m_hotkeyFilter;
    SettingsWindow *m_settingsWindow;
    /// @brief Absolute path to the JSON config file used for load/save.
    QString m_configPath;
    /// @brief Persistent MRU + frequency cache beside the menu config.
    ActionHistory m_actionHistory;
    /// @brief Owns worker + scheduler threads for Action execution.
    std::unique_ptr<Scheduler> m_scheduler;
    /// @brief Polls GetAsyncKeyState while waiting for hotkey release.
    QTimer *m_releaseWatchTimer{nullptr};
    bool m_executeOnReleaseArmed{false};
    int m_releaseWatchMod{0};
    int m_releaseWatchVk{0};
    /// @brief Polls Escape while the wheel is visible (Qt keys never reach it).
    QTimer *m_escapeWatchTimer{nullptr};
    /// @brief Prior Escape-down sample for rising-edge dismiss detection.
    bool m_escapeWasDown{false};
    /// @brief True while settings has unregistered hotkeys for key capture.
    bool m_hotkeysSuspended{false};

    /// @brief Path of action_history.json next to the menu config.
    [[nodiscard]] QString historyPath() const;
    /// @brief Drops history entries that no longer exist in the library.
    void pruneActionHistory();
    /// @brief Persists action history to disk (best-effort).
    void saveActionHistory();
    /// @brief Application level configuration like global hotkey.
    AppConfig m_appConfig;
};

} // namespace Application
