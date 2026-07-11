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

#include <vector>
#include <string>
#include <memory>

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QResizeEvent>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPushButton>

#include "App/Menu.hpp"
#include "App/Action.hpp"
#include "App/RadialMenuWidget.hpp"

namespace Application
{

    class Gui : public QWidget
    {
        Q_OBJECT

    public:
        explicit Gui(QWidget *parent = nullptr);

        /**
         * @brief Called when the selection index for Menu changes
         *
         * @param selectionInd The new value of the index, -1 if unset
         */
        void onSelectChange(int selectionInd);
        /**
         * @brief Updates the GUI to a new menu
         *
         * @param menu reference to the new menu
         */
        void setMenu(const Menu &menu);

    signals:
        /**
         * @brief Emits signal on escape key being pressed
         *
         * TODO: Pretty sure this only triggers when app open hopefully
         *
         */
        void escapePressed();

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;

    private:
        QLabel *m_titleLabel{nullptr};
        RadialMenuWidget *m_radialMenu{nullptr};
        QPushButton *m_settingsButton{nullptr};
        QPushButton *m_editButton{nullptr};
    };
}