/**
 * @file Gui.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-07-04
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once
#include <vector>
#include <QWidget>
#include <Platform/Window.hpp>
#include <QPushButton>
#include <QLabel>

namespace Application
{
    class Gui : public QWidget
    {
    public:
        explicit Gui(QWidget *parent = 0);
        QLabel *label;
    };

    class HoverButton : public QPushButton
    {
        Q_OBJECT
    public:
        HoverButton(QWidget *parent = nullptr) : QPushButton(parent) {}
    signals:
        void mouseHovered();
        void mouseLeft();

    protected:
        void enterEvent(QEnterEvent *event) override
        {
            QPushButton::enterEvent(event);
            emit mouseHovered();
        }
        void leaveEvent(QEvent *event) override
        {
            QPushButton::leaveEvent(event);
            emit mouseLeft();
        }
    };
}