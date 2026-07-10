#include "App/HoverButton.hpp"
using namespace Application;

// ---------------- HoverButton ----------------

HoverButton::HoverButton(QWidget *parent)
    : QPushButton(parent)
{
}

void HoverButton::enterEvent(QEnterEvent *event)
{
    QPushButton::enterEvent(event);
    emit mouseHovered();
}

void HoverButton::leaveEvent(QEvent *event)
{
    QPushButton::leaveEvent(event);
    emit mouseLeft();
}