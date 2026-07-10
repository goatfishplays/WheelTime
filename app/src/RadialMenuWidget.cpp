#include "App/RadialMenuWidget.hpp"

#include <cmath>
#include <QLabel>
#include <QStyle>

using namespace Application;

RadialMenuWidget::RadialMenuWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_Hover, true);

    m_centerLabel = new QLabel(this);
    m_centerLabel->setAlignment(Qt::AlignCenter);
}

RadialMenuWidget::RadialMenuWidget(const Menu &menu, QWidget *parent)
    : RadialMenuWidget(parent)
{
    setMenu(menu, {});
}

void RadialMenuWidget::setMenu(const Menu &menu, const std::vector<std::string> &actionLabels)
{
    m_menu = menu;
    m_actionLabels = actionLabels;
    setCenterText(QString::fromStdString(menu.getName()));
    setSelectedIndex(-1);
    rebuildButtons();
}

void RadialMenuWidget::setCenterText(const QString &text)
{
    m_centerLabel->setText(text);
    m_centerLabel->adjustSize();
    update();
}

void RadialMenuWidget::setButtonRadius(int radius)
{
    m_buttonRadius = radius;
    repositionButtons();
}

void RadialMenuWidget::setActivationMode(ActivationMode mode)
{
    m_mode = mode;
    clearSelection();
}

void RadialMenuWidget::setMaxDistance(double maxDistance)
{
    m_maxDistance = maxDistance;
}

int RadialMenuWidget::getSelectedIndex() const
{
    return m_selectedIndex;
}

void RadialMenuWidget::setSelectedIndex(int newVal)
{
    HoverButton *button = nullptr;
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_buttons.size()))
    {
        button = m_buttons[m_selectedIndex];
        button->setProperty("selectedAction", false);
        button->style()->unpolish(button);
        button->style()->polish(button);
        button->update();
    }

    m_selectedIndex = newVal;
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_buttons.size()))
    {
        button = m_buttons[m_selectedIndex];
        button->setProperty("selectedAction", true);
        button->style()->unpolish(button);
        button->style()->polish(button);
        button->update();
    }

    emit selectedIndexChanged(m_selectedIndex);
}

void RadialMenuWidget::rebuildButtons()
{
    for (HoverButton *button : m_buttons)
    {
        button->deleteLater();
    }
    m_buttons.clear();

    for (int i = 0; i < static_cast<int>(m_actionLabels.size()); ++i)
    {
        auto *button = new HoverButton(this);
        button->setText(QString::fromStdString(m_actionLabels[i]));
        button->adjustSize();
        button->show();

        connect(button, &HoverButton::mouseHovered, this, [this, i]()
                {
                    if (m_mode == ActivationMode::Exact && m_selectedIndex != i)
                    {
                        setSelectedIndex(i);
                    } });

        connect(button, &HoverButton::mouseLeft, this, [this, i]()
                {
                    if (m_mode == ActivationMode::Exact && m_selectedIndex == i)
                    {
                        clearSelection();
                    } });

        connect(button, &QPushButton::clicked, this, [this, i]()
                { emit buttonTriggered(i); });

        m_buttons.push_back(button);
    }

    repositionButtons();
}

void RadialMenuWidget::repositionButtons()
{
    const int count = static_cast<int>(m_buttons.size());
    if (count == 0)
    {
        m_centerLabel->adjustSize();
        m_centerLabel->move(
            static_cast<int>(width() * 0.5 - m_centerLabel->width() * 0.5),
            static_cast<int>(height() * 0.5 - m_centerLabel->height() * 0.5));
        return;
    }

    const QPointF center(width() * 0.5, height() * 0.5);
    const double radius = static_cast<double>(m_buttonRadius);

    for (int i = 0; i < count; ++i)
    {
        const double angle = (-M_PI / 2.0) + (2.0 * M_PI * i / count);

        HoverButton *button = m_buttons[i];
        button->adjustSize();

        const int x = static_cast<int>(center.x() + radius * std::cos(angle) - button->width() * 0.5);
        const int y = static_cast<int>(center.y() + radius * std::sin(angle) - button->height() * 0.5);
        button->move(x, y);
    }

    m_centerLabel->adjustSize();
    m_centerLabel->move(
        static_cast<int>(center.x() - m_centerLabel->width() * 0.5),
        static_cast<int>(center.y() - m_centerLabel->height() * 0.5));
}

void RadialMenuWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    repositionButtons();
}

void RadialMenuWidget::updateSelectionFromGlobalMousePosition(const QPoint &globalPos)
{
    m_mousePosition = mapFromGlobal(globalPos);
    updateSelection();
}

void RadialMenuWidget::clearSelection()
{
    if (m_selectedIndex != -1)
    {
        setSelectedIndex(-1);
    }
}

int RadialMenuWidget::indexFromAngle(double angleRadians, int count) const
{
    const double slice = 2.0 * M_PI / count;
    int idx = static_cast<int>(std::round(angleRadians / slice)) % count;
    if (idx < 0)
    {
        idx += count;
    }
    return idx;
}

void RadialMenuWidget::updateSelection()
{
    const QPointF center = rect().center();
    const double dx = m_mousePosition.x() - center.x();
    const double dy = m_mousePosition.y() - center.y();
    const double dist2 = dx * dx + dy * dy;

    const int count = static_cast<int>(m_buttons.size());
    if (count == 0)
    {
        clearSelection();
        return;
    }

    if (m_maxDistance >= 0.0 && dist2 > m_maxDistance * m_maxDistance)
    {
        clearSelection();
        return;
    }

    if (m_mode == ActivationMode::Distance)
    {
        int best = -1;
        double bestScore = 0.0;

        for (int i = 0; i < count; ++i)
        {
            const QPoint centerPoint = m_buttons[i]->geometry().center();
            const double ddx = m_mousePosition.x() - centerPoint.x();
            const double ddy = m_mousePosition.y() - centerPoint.y();
            const double score = ddx * ddx + ddy * ddy;

            if (best == -1 || score < bestScore)
            {
                best = i;
                bestScore = score;
            }
        }

        if (m_selectedIndex != best)
        {
            setSelectedIndex(best);
        }
        return;
    }

    if (m_mode == ActivationMode::Angle)
    {
        double angle = std::atan2(dy, dx);
        angle += M_PI / 2.0;

        while (angle < 0.0)
        {
            angle += 2.0 * M_PI;
        }
        while (angle >= 2.0 * M_PI)
        {
            angle -= 2.0 * M_PI;
        }

        const int best = indexFromAngle(angle, count);
        if (m_selectedIndex != best)
        {
            setSelectedIndex(best);
        }
    }
}

bool RadialMenuWidget::eventFilter(QObject *watched, QEvent *event)
{
    return QWidget::eventFilter(watched, event);
}
