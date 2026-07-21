/**
 * @file Gui.hpp
 * @brief Main launcher GUI host for the radial wheel.
 */
#pragma once

#include <vector>
#include <string>
#include <memory>

#include <QWidget>
#include <QKeyEvent>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QResizeEvent>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QPushButton>
#include <QDialog>
#include <QGridLayout>

#include "App/Menu.hpp"
#include "App/Action.hpp"
#include "App/RadialMenuWidget.hpp"
#include "App/SearchConfig.hpp"

namespace Application
{
    class SettingsWindow;
    class SearchPaletteWidget;

    /**
     * @brief Hosts the wheel widget and top-level launcher controls.
     *
     * `Gui` is intentionally thin: it shows a menu, forwards selection changes,
     * and exposes a couple of high-level signals while the heavier editing logic
     * lives in `SettingsWindow`.
     */
    class Gui : public QWidget
    {
        Q_OBJECT

    public:
        explicit Gui(QWidget *parent = nullptr);
        /// @brief Updates hover/selection presentation for the active slot.
        void onSelectChange(int selectionInd);
        /// @brief Displays a menu plus the resolved labels/icons for its action slots.
        void setMenu(const Menu &menu, const std::vector<ActionSlotVisual> &slotVisuals);
        /// @brief Puts the overlay into visible, mouse-interactive mode.
        void enterInteractiveOverlay();
        /// @brief Hides launcher visuals and leaves the shell alive for click-through mode.
        void enterDormantOverlay();
        /// @brief Shows the settings editor inside the overlay shell.
        void showSettingsPanel(SettingsWindow *settingsWindow);
        /// @brief Hides the embedded settings editor without destroying it.
        void hideSettingsPanel();
        /// @brief Shows the search palette inside the overlay shell.
        void showSearchPanel(const SearchConfig &config);
        /// @brief Hides the search palette without destroying it.
        void hideSearchPanel();
        /// @brief Puts keyboard focus in the search palette's input field.
        void focusSearchInput();
        /// @brief Kicks off the background program-index scan at startup so
        /// the first search open is instant.
        void preloadSearchIndex();
        /// @brief Whether the launcher visuals are currently visible.
        bool isLauncherVisible() const;
        /// @brief Whether the overlay is currently showing the embedded settings editor.
        bool isSettingsVisible() const;
        /// @brief Whether the overlay is currently showing the search palette.
        bool isSearchVisible() const;
        /// @brief Seeds Distance-mode selection from the current cursor position.
        void refreshSelectionFromCursor();
        /// @brief Currently selected wheel slot, or -1 when none.
        [[nodiscard]] int selectedActionIndex() const;

    signals:
        /// @brief Emitted when the launcher should be dismissed with Escape.
        void escapePressed();

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;

    private:
        enum class OverlayMode
        {
            Dormant,
            Wheel,
            Settings,
            Search
        };

        QWidget *m_overlayPanel{nullptr};
        QWidget *m_searchHost{nullptr};
        SearchPaletteWidget *m_searchPalette{nullptr};
        QWidget *m_settingsHost{nullptr};
        QGridLayout *m_settingsHostLayout{nullptr};
        QLabel *m_titleLabel{nullptr};
        RadialMenuWidget *m_radialMenu{nullptr};
        QPushButton *m_settingsButton{nullptr};
        OverlayMode m_overlayMode{OverlayMode::Dormant};
    };
}
