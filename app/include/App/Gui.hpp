/**
 * @file Gui.hpp
 * @brief The main gui interface class
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
    class Gui : public QWidget
    {
        Q_OBJECT

    public:
        explicit Gui(QWidget *parent = nullptr);
        void onSelectChange(int selectionInd);
        void setMenu(const Menu &menu, const std::vector<std::string> &actionLabels);

    signals:
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
