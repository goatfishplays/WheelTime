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

#include "App/Menu.hpp"
#include "App/Action.hpp"
#include "App/RadialMenuWidget.hpp"

namespace Application
{
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
        /// @brief Whether the launcher visuals are currently visible.
        bool isLauncherVisible() const;

    signals:
        /// @brief Emitted when the launcher should be dismissed with Escape.
        void escapePressed();

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;

    private:
        QWidget *m_overlayPanel{nullptr};
        QLabel *m_titleLabel{nullptr};
        RadialMenuWidget *m_radialMenu{nullptr};
        QPushButton *m_settingsButton{nullptr};
        /// @brief Kept for now so the older UI affordance still exists visually.
        QPushButton *m_editButton{nullptr};
        bool m_launcherVisible{false};
    };
}
