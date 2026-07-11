/**
 * @file Gui.hpp
 * @author your name (you@domain.com)
 * @brief The main gui interface class
 * @version 0.1
 * @date 2026-07-09
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QWidget>
#include <vector>

#include "App/Menu.hpp"
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
        /// @brief Displays a menu plus the resolved labels for its action slots.
        void setMenu(const Menu &menu, const std::vector<std::string> &actionLabels);

    signals:
        /// @brief Emitted when the launcher should be dismissed with Escape.
        void escapePressed();

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;

    private:
        QLabel *m_titleLabel{nullptr};
        RadialMenuWidget *m_radialMenu{nullptr};
        QPushButton *m_settingsButton{nullptr};
        /// @brief Kept for now so the older UI affordance still exists visually.
        QPushButton *m_editButton{nullptr};
    };
}
