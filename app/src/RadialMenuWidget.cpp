#include "App/RadialMenuWidget.hpp"

#include <algorithm>
#include <cmath>
#include <QCursor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

using namespace Application;

// ---------------- RadialMenuWidget ----------------
RadialMenuWidget::RadialMenuWidget(QWidget *parent)
    : QWidget(parent)
{
    // Distance mode relies on MouseMove while no button is held.
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);

    m_centerLabel = new QLabel(this);
    m_centerLabel->setObjectName("radialCenterLabel");
    m_centerLabel->setAlignment(Qt::AlignCenter);
}

RadialMenuWidget::RadialMenuWidget(const Menu &menu, QWidget *parent)
    : RadialMenuWidget(parent)
{
    setMenu(menu, {});
}

void RadialMenuWidget::setMenu(const Menu &menu, const std::vector<ActionSlotVisual> &slotVisuals)
{
    // Hotkey open calls setMenu every time; skip a full tear-down/rebuild when
    // nothing visible changed (icon decode + QToolButton construction is costly).
    const bool unchanged = m_menu.getId() == menu.getId() && m_slotVisuals == slotVisuals
        && static_cast<int>(m_buttons.size()) == static_cast<int>(slotVisuals.size());
    m_menu = menu;
    m_slotVisuals = slotVisuals;

    if (unchanged)
    {
        setSelectedIndex(-1);
        return;
    }

    m_selectedIndex = -1;
    setCenterText(QString::fromStdString(menu.getName()));
    rebuildButtons();
}

void RadialMenuWidget::setCenterText(const QString &text)
{
    m_centerLabel->setText(text);
    m_centerLabel->ensurePolished();
    m_centerLabel->adjustSize();
    placeWidgetCentered(m_centerLabel, layoutCenter());
    update();
}

void RadialMenuWidget::setButtonRadiusFraction(double fraction)
{
    m_buttonRadiusFraction = std::clamp(fraction, 0.15, 0.48);
    m_useFixedButtonRadius = false;
    repositionButtons();
}

void RadialMenuWidget::setButtonRadius(int radius)
{
    m_buttonRadius = std::max(1, radius);
    m_useFixedButtonRadius = true;
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

    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_slotVisuals.size()))
    {
        setCenterText(QString::fromStdString(m_slotVisuals[m_selectedIndex].label));
    }
    else
    {
        setCenterText(QString::fromStdString(m_menu.getName()));
    }

    // Size can change with selection styling; recentre slots on the ring.
    repositionButtons();

    emit selectedIndexChanged(m_selectedIndex);
}

void RadialMenuWidget::rebuildButtons()
{
    for (HoverButton *button : m_buttons)
    {
        button->deleteLater();
    }
    m_buttons.clear();

    for (int i = 0; i < static_cast<int>(m_slotVisuals.size()); ++i)
    {
        auto *button = new HoverButton(this);
        const ActionSlotVisual &visual = m_slotVisuals[i];
        const QString label = QString::fromStdString(visual.label);
        button->setText(label);
        button->setToolTip(label);
        if (!visual.iconPath.empty())
        {
            const QIcon icon = iconForPath(visual.iconPath);
            if (!icon.isNull())
            {
                button->setIcon(icon);
                // Icon-only so a larger image fills the existing button chrome
                // without growing the control for under-icon text (name still
                // appears in the wheel center / tooltip).
                button->setToolButtonStyle(Qt::ToolButtonIconOnly);
            }
        }
        // Child widgets need their own tracking; parent tracking does not apply
        // while the cursor is over a button.
        button->setMouseTracking(true);
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

        connect(button, &QToolButton::clicked, this, [this, i]()
                { emit buttonTriggered(i); });

        m_buttons.push_back(button);
    }

    repositionButtons();
}

void RadialMenuWidget::repositionButtons()
{
    const QPointF center = layoutCenter();
    const int count = static_cast<int>(m_buttons.size());
    if (count == 0)
    {
        m_centerLabel->ensurePolished();
        m_centerLabel->adjustSize();
        placeWidgetCentered(m_centerLabel, center);
        return;
    }

    const double radius = static_cast<double>(effectiveButtonRadius());

    // Angles are measured from the top and increase clockwise.
    // 1 button: 12:00
    // 2 buttons: 12:00, 6:00
    // 3 buttons: 12:00, 4:00, 8:00
    // 4 buttons: 12:00, 3:00, 6:00, 9:00
    // etc.
    for (int i = 0; i < count; ++i)
    {
        const double angle = (-M_PI / 2.0) + (2.0 * M_PI * i / count);

        HoverButton *button = m_buttons[i];
        button->ensurePolished();
        button->adjustSize();

        const QPointF slotCenter(
            center.x() + radius * std::cos(angle),
            center.y() + radius * std::sin(angle));
        placeWidgetCentered(button, slotCenter);
    }

    // Center label stays on the same layout origin as the ring.
    m_centerLabel->ensurePolished();
    m_centerLabel->adjustSize();
    placeWidgetCentered(m_centerLabel, center);
}

QPointF RadialMenuWidget::layoutCenter() const
{
    return {width() * 0.5, height() * 0.5};
}

QPointF RadialMenuWidget::widgetCenter(const QWidget *widget)
{
    return {
        widget->x() + widget->width() * 0.5,
        widget->y() + widget->height() * 0.5};
}

void RadialMenuWidget::placeWidgetCentered(QWidget *widget, QPointF center)
{
    widget->move(
        qRound(center.x() - widget->width() * 0.5),
        qRound(center.y() - widget->height() * 0.5));
}

int RadialMenuWidget::effectiveButtonRadius() const
{
    if (m_useFixedButtonRadius)
    {
        return m_buttonRadius;
    }

    // Scale with the radial panel (monitor-sized after title/footer chrome).
    // Leave margin so enlarged selected buttons don't clip the edges.
    const int shortSide = std::min(width(), height());
    const int edgeMargin = 80;
    const int scaled = qRound(shortSide * m_buttonRadiusFraction);
    const int maxRadius = std::max(60, shortSide / 2 - edgeMargin);
    return std::clamp(scaled, 60, maxRadius);
}

QIcon RadialMenuWidget::iconForPath(const std::string &path) const
{
    if (path.empty())
    {
        return {};
    }

    const auto cached = m_iconCache.find(path);
    if (cached != m_iconCache.end())
    {
        return cached->second;
    }

    QIcon icon(QString::fromStdString(path));
    m_iconCache.emplace(path, icon);
    return icon;
}

void RadialMenuWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    repositionButtons();
}
// void RadialMenuWidget::mouseMoveEvent(QMouseEvent *event)
// {
//     // updateSelectionFromMouse(event->position().toPoint());
//     m_mousePosition = event->position().toPoint();
//     updateSelection();
//     qDebug() << "Mouse pos: " << m_mousePosition;

//     QWidget::mouseMoveEvent(event);
// }

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
    // angleRadians should already be normalized to [0, 2pi).
    // Buckets the cursor into the nearest menu slice.
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
    const QPointF center = layoutCenter();
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
            const QPointF centerPoint = widgetCenter(m_buttons[i]);
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
        // angle 0 = 12:00, then clockwise
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
    // Exact mode is handled by hover events on the buttons.
}

bool RadialMenuWidget::eventFilter(QObject *watched, QEvent *event)
{
    return QWidget::eventFilter(watched, event);
}
