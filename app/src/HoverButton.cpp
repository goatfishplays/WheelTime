#include "App/HoverButton.hpp"

using namespace Application;

HoverButton::HoverButton(QWidget *parent)
    : QToolButton(parent)
{
    setObjectName("wheelActionButton");
    setProperty("selectedAction", false);
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    setAutoRaise(false);
}

void HoverButton::enterEvent(QEnterEvent *event)
{
    QToolButton::enterEvent(event);
    emit mouseHovered();
}

void HoverButton::leaveEvent(QEvent *event)
{
    QToolButton::leaveEvent(event);
    emit mouseLeft();
}
