#include "App/Gui.hpp"

#include <cmath>
#include <QCursor>
#include <QMouseEvent>
#include <QApplication>
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
        m_radialMenu->updateSelectionFromGlobalMousePosition(mouseMoveEvent->globalPosition().toPoint());
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