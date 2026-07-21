/**
 * @file HoverButton.hpp
 * @brief Tool button with hover signals for radial-menu slots.
 */
#pragma once

#include <QEnterEvent>
#include <QEvent>
#include <QToolButton>

namespace Application
{

class HoverButton : public QToolButton
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

} // namespace Application
