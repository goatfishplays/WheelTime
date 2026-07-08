#include "App/Gui.hpp"

#include <cmath>
#include <QCursor>
#include <QMouseEvent>
#include <QApplication>
#include <QStyle>
#include <QDebug>
// TODO: segment this into multiple files
#include <Platform/Execute.hpp>
#include "App/App.hpp"

using namespace Application;

static void runTestAction(int index)
{
    Platform::Executor executor;

    switch (index)
    {
    case 0:
        qDebug() << "Running test action: Notepad";
        executor.executeScript("notepad.exe");
        break;

    case 1:
        qDebug() << "Running test action: Calculator";
        executor.executeScript("calc.exe");
        break;

    default:
        qDebug() << "No test action assigned for index:" << index;
        break;
    }
}

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

// ---------------- RadialMenuWidget ----------------

RadialMenuWidget::RadialMenuWidget(QWidget *parent)
    : QWidget(parent)
{
    // setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);

    m_centerLabel = new QLabel(this);
    m_centerLabel->setAlignment(Qt::AlignCenter);
}

RadialMenuWidget::RadialMenuWidget(const Menu &menu, QWidget *parent)
    : RadialMenuWidget(parent)
{
    setMenu(menu);
}

void RadialMenuWidget::setMenu(const Menu &menu)
{
    m_menu = menu;
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
    HoverButton *button;
    if (m_selectedIndex >= 0)
    {
        button = m_buttons[m_selectedIndex];
        button->setProperty("selectedAction", false);
        button->style()->unpolish(button);
        button->style()->polish(button);
        button->update();
    }

    m_selectedIndex = newVal;
    if (m_selectedIndex >= 0)
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

    for (int i = 0; i < static_cast<int>(m_menu.actions.size()); ++i)
    {
        auto *button = new HoverButton(this);
        button->setText(QString::fromStdString(m_menu.actions[i].getName()));
        button->adjustSize();
        button->show();

        connect(button, &HoverButton::mouseHovered, this, [this, i]()
                {
            if (m_mode == ActivationMode::Exact)
            {
                if (m_selectedIndex != i)
                {
                    setSelectedIndex(i);
                }
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
        return;

    const QPointF center(width() * 0.5, height() * 0.5);
    const double radius = static_cast<double>(m_buttonRadius);

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
        button->adjustSize();

        const int x = static_cast<int>(center.x() + radius * std::cos(angle) - button->width() * 0.5);
        const int y = static_cast<int>(center.y() + radius * std::sin(angle) - button->height() * 0.5);
        button->move(x, y);
    }

    // Center label stays centered.
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
// void RadialMenuWidget::mouseMoveEvent(QMouseEvent *event)
// {
//     // updateSelectionFromMouse(event->position().toPoint());
//     m_mousePosition = event->position().toPoint();
//     updateSelection();
//     qDebug() << "Mouse pos: " << m_mousePosition;

//     QWidget::mouseMoveEvent(event);
// }

void RadialMenuWidget::setMousePosFromGlobal(const QPoint &globalPos)
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
        idx += count;
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
            const QPoint c = m_buttons[i]->geometry().center(); // local coords
            const double ddx = m_mousePosition.x() - c.x();
            const double ddy = m_mousePosition.y() - c.y();
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
            angle += 2.0 * M_PI;
        while (angle >= 2.0 * M_PI)
            angle -= 2.0 * M_PI;

        const int best = indexFromAngle(angle, count);

        if (m_selectedIndex != best)
        {
            setSelectedIndex(best);
        }
        return;
    }

    // Exact mode is handled by hover events on the buttons.
}

bool RadialMenuWidget::eventFilter(QObject *watched, QEvent *event)
{
    return QWidget::eventFilter(watched, event);
}

// ---------------- Gui ----------------

Gui::Gui(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(600, 400);
    // showFullScreen();

    // setAttribute(Qt::WA_TranslucentBackground);
    // setWindowFlags(Qt::FramelessWindowHint);

    // setStyleSheet(R"(
    //     QWidget {
    //         background: transparent;
    //     }
    // )");

    qApp->installEventFilter(this);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // Top title
    m_titleLabel = new QLabel("Title", this);
    m_titleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    root->addWidget(m_titleLabel);

    // Middle area that holds the radial widget and can expand freely.
    auto *middle = new QWidget(this);
    auto *middleLayout = new QVBoxLayout(middle);
    middleLayout->setContentsMargins(0, 0, 0, 0);

    m_radialMenu = new RadialMenuWidget(middle);
    m_radialMenu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    middleLayout->addWidget(m_radialMenu);

    root->addWidget(middle, 1);

    // Bottom row: spacer pushes buttons to the right.
    auto *bottomRow = new QHBoxLayout();
    bottomRow->addStretch();

    m_settingsButton = new QPushButton("Settings", this);
    m_editButton = new QPushButton("Edit", this);

    bottomRow->addWidget(m_settingsButton);
    bottomRow->addWidget(m_editButton);

    root->addLayout(bottomRow);

    // example = "Example Menu";
    // example.actions = {
    //     {"One"},
    //     {"Two"},
    //     {"Three"},
    //     {"Four"}};

    // App::App::getInstance().loadedMenus.push_back(example);
    // App::App::getInstance().showGui(&App::App::getInstance().loadedMenus[0]);
    // m_radialMenu->setMenu(example);
    m_radialMenu->setButtonRadius(100);
    m_radialMenu->setActivationMode(RadialMenuWidget::ActivationMode::Distance);

    connect(m_radialMenu, &RadialMenuWidget::selectedIndexChanged, this, &Gui::onSelectChange);

    connect(m_radialMenu, &RadialMenuWidget::buttonTriggered, this, [](int index)
            { qDebug() << "Button clicked:" << index;
            runTestAction(index); });
}

void Gui::onSelectChange(int index)
{
    qDebug() << "Selected index changed:" << index;
    // m_radialMenu->highlightButton(m_radialMenu->getSelectedIndex());
};

bool Gui::eventFilter(QObject *watched, QEvent *event)
{
    // Only handle events that pertain to this object
    // if (watched != this)
    //     return false;

    if (event->type() == QEvent::MouseMove)
    {
        auto *mouseMoveEvent = static_cast<QMouseEvent *>(event);
        m_radialMenu->setMousePosFromGlobal(mouseMoveEvent->globalPosition().toPoint());
        // m_radialMenu->updateSelectionFromGlobal(mouseEvent->globalPosition().toPoint());
    }
    if (event->type() == QEvent::MouseButtonPress)
    {
        qDebug() << "Event detected, obj: " << watched << " | event: " << event;
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            App::App::getInstance().executeAction(m_radialMenu->getSelectedIndex());
            return true;
        }
        if (mouseEvent->button() == Qt::RightButton)
        {
            App::App::getInstance().hideGui();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
    // this->show();
}

void Gui::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        emit escapePressed();
    }
    else
    {
        QWidget::keyPressEvent(event);
    }
}

void Gui::setMenu(const Menu &menu) // TODO: might be better to just make the radial menu public
{
    m_radialMenu->setMenu(menu);
}