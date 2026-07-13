#include "App/Gui.hpp"
// TODO: segment this into multiple files
#include <QApplication>
#include <QDebug>
#include <QCursor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <cmath>

#include "App/App.hpp"
#include "App/Action.hpp"

using namespace Application;

Gui::Gui(QWidget *parent)
    : QWidget(parent)
{
    // The overlay shell stays alive for the app lifetime. Native window styles
    // later make it non-activating/topmost/click-through when dormant.
    setObjectName("launcherOverlay");
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setFocusPolicy(Qt::NoFocus);

    qApp->installEventFilter(this);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    root->addStretch();
    auto *centerRow = new QHBoxLayout();
    centerRow->addStretch();

    // This centered panel contains the existing wheel UI while the top-level
    // widget stretches fullscreen across the active monitor.
    m_overlayPanel = new QWidget(this);
    m_overlayPanel->setObjectName("launcherPanel");
    m_overlayPanel->setFixedSize(600, 400);
    m_overlayPanel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    centerRow->addWidget(m_overlayPanel);
    centerRow->addStretch();
    root->addLayout(centerRow);
    root->addStretch();

    auto *panelLayout = new QVBoxLayout(m_overlayPanel);
    panelLayout->setContentsMargins(12, 12, 12, 12);
    panelLayout->setSpacing(8);

    // Top title
    m_titleLabel = new QLabel("Title", m_overlayPanel);
    m_titleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    panelLayout->addWidget(m_titleLabel);

    // Middle area that holds the radial widget and can expand freely.
    auto *middle = new QWidget(m_overlayPanel);
    auto *middleLayout = new QVBoxLayout(middle);
    middleLayout->setContentsMargins(0, 0, 0, 0);

    m_radialMenu = new RadialMenuWidget(middle);
    m_radialMenu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    middleLayout->addWidget(m_radialMenu);

    panelLayout->addWidget(middle, 1);

    // Bottom row: spacer pushes buttons to the right.
    auto *bottomRow = new QHBoxLayout();
    bottomRow->addStretch();

    m_settingsButton = new QPushButton("Settings", m_overlayPanel);
    m_editButton = new QPushButton("Edit", m_overlayPanel);
    bottomRow->addWidget(m_settingsButton);
    bottomRow->addWidget(m_editButton);
    panelLayout->addLayout(bottomRow);

    m_radialMenu->setButtonRadius(100);
    m_radialMenu->setActivationMode(RadialMenuWidget::ActivationMode::Distance);

    connect(m_radialMenu, &RadialMenuWidget::selectedIndexChanged, this, &Gui::onSelectChange);
    connect(m_settingsButton, &QPushButton::clicked, []()
            { App::getInstance().showSettingsWindow(); });
    connect(m_editButton, &QPushButton::clicked, []()
            { App::getInstance().showSettingsWindow(); });
    connect(m_radialMenu, &RadialMenuWidget::buttonTriggered, this, [](int index)
            { App::getInstance().executeAction(index); });

    enterDormantOverlay();
}

void Gui::onSelectChange(int index)
{
    qDebug() << "Selected index changed:" << index;
}

bool Gui::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_launcherVisible)
    {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseMove)
    {
        auto *mouseMoveEvent = static_cast<QMouseEvent *>(event);
        m_radialMenu->updateSelectionFromGlobalMousePosition(mouseMoveEvent->globalPosition().toPoint());
    }

    if (event->type() == QEvent::MouseButtonPress)
    {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QPoint localPos = mapFromGlobal(mouseEvent->globalPosition().toPoint());
        QWidget *clickedChild = childAt(localPos);

        if (mouseEvent->button() == Qt::LeftButton)
        {
            if (clickedChild == m_settingsButton || clickedChild == m_editButton)
            {
                return QWidget::eventFilter(watched, event);
            }

            if (qobject_cast<QPushButton *>(watched) != nullptr || qobject_cast<QPushButton *>(clickedChild) != nullptr)
            {
                return QWidget::eventFilter(watched, event);
            }

            if (m_radialMenu->geometry().contains(localPos))
            {
                App::getInstance().executeAction(m_radialMenu->getSelectedIndex());
                return true;
            }

            return QWidget::eventFilter(watched, event);
        }

        if (mouseEvent->button() == Qt::RightButton)
        {
            App::getInstance().hideGui();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
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

void Gui::setMenu(const Menu &menu, const std::vector<std::string> &actionLabels)
{
    m_titleLabel->setText(QString::fromStdString(menu.getName()));
    m_radialMenu->setMenu(menu, actionLabels);
}

void Gui::enterInteractiveOverlay()
{
    m_launcherVisible = true;
    if (m_overlayPanel != nullptr)
    {
        m_overlayPanel->show();
    }
}

void Gui::enterDormantOverlay()
{
    m_launcherVisible = false;
    if (m_overlayPanel != nullptr)
    {
        m_overlayPanel->hide();
    }
}

bool Gui::isLauncherVisible() const
{
    return m_launcherVisible;
}
