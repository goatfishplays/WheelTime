/**
 * @file HoverButton.hpp
 * @author your name (you@domain.com)
 * @brief Push button extended to add hover signals
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

namespace Application
{

    class HoverButton : public QPushButton
    {
        Q_OBJECT
    public:
        explicit HoverButton(QWidget *parent = nullptr);

    signals:
        void mouseHovered();
        void mouseLeft();

    protected:
        void enterEvent(QEnterEvent *event) override;
        void leaveEvent(QEvent *event) override;
    };
}