#include "App/Gui.hpp"
// TODO: segment this into multiple files
#include <QAbstractButton>
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
#include "App/SearchPaletteWidget.hpp"
#include "App/SettingsWindow.hpp"

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
    setMouseTracking(true);

    qApp->installEventFilter(this);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Panel fills the fullscreen overlay (which is sized to the active monitor).
    m_overlayPanel = new QWidget(this);
    m_overlayPanel->setObjectName("launcherPanel");
    m_overlayPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_overlayPanel->setMouseTracking(true);
    root->addWidget(m_overlayPanel);

    auto *panelLayout = new QVBoxLayout(m_overlayPanel);
    panelLayout->setContentsMargins(12, 12, 12, 12);
    panelLayout->setSpacing(8);

    // Top title
    m_titleLabel = new QLabel("Title", m_overlayPanel);
    m_titleLabel->setObjectName("launcherTitle");
    m_titleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    panelLayout->addWidget(m_titleLabel);

    // Middle area that holds the radial widget and can expand freely.
    auto *middle = new QWidget(m_overlayPanel);
    middle->setObjectName("launcherPanelBody");
    middle->setMouseTracking(true);
    auto *middleLayout = new QVBoxLayout(middle);
    middleLayout->setContentsMargins(0, 0, 0, 0);

    m_radialMenu = new RadialMenuWidget(middle);
    m_radialMenu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_radialMenu->setMouseTracking(true);
    middleLayout->addWidget(m_radialMenu);

    panelLayout->addWidget(middle, 1);

    // Bottom row: spacer pushes buttons to the right.
    auto *bottomRow = new QHBoxLayout();
    bottomRow->addStretch();

    m_settingsButton = new QPushButton("Settings", m_overlayPanel);
    bottomRow->addWidget(m_settingsButton);
    panelLayout->addLayout(bottomRow);

    // Ring radius tracks the radial panel size (monitor body after chrome).
    m_radialMenu->setButtonRadiusFraction(0.32);
    m_radialMenu->setActivationMode(RadialMenuWidget::ActivationMode::Distance);

    connect(m_radialMenu, &RadialMenuWidget::selectedIndexChanged, this, &Gui::onSelectChange);
    connect(m_settingsButton, &QPushButton::clicked, []()
            { App::getInstance().showSettingsWindow(); });
    connect(m_radialMenu, &RadialMenuWidget::buttonTriggered, this, [](int index)
            { App::getInstance().executeAction(index); });

    // Settings shares the long-lived overlay shell so it feels like part of
    // WheelTime instead of an unrelated always-on-top window. It is only shown
    // in settings mode; dormant mode still hides all graphics and becomes
    // click-through at the native window layer.
    m_settingsHost = new QWidget(this);
    m_settingsHost->setObjectName("settingsOverlayHost");
    m_settingsHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    root->addWidget(m_settingsHost);

    m_settingsHostLayout = new QGridLayout(m_settingsHost);
    m_settingsHostLayout->setContentsMargins(48, 36, 48, 36);
    m_settingsHostLayout->setSpacing(0);
    m_settingsHost->hide();

    // Search palette shares the overlay shell as a third mode. Unlike the
    // wheel it needs keyboard focus; App flips the native window out of
    // no-activate mode while it is open, like it does for settings.
    m_searchHost = new QWidget(this);
    m_searchHost->setObjectName("searchOverlayHost");
    m_searchHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    root->addWidget(m_searchHost);

    auto *searchLayout = new QVBoxLayout(m_searchHost);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(0);
    m_searchPalette = new SearchPaletteWidget(m_searchHost);
    auto *searchRow = new QHBoxLayout();
    searchRow->addStretch();
    searchRow->addWidget(m_searchPalette);
    searchRow->addStretch();
    // Upper-middle placement, walker/rofi style.
    searchLayout->addStretch(1);
    searchLayout->addLayout(searchRow);
    searchLayout->addStretch(2);
    m_searchHost->hide();

    enterDormantOverlay();
}

void Gui::onSelectChange(int index)
{
    qDebug() << "Selected index changed:" << index;
}

bool Gui::eventFilter(QObject *watched, QEvent *event)
{
    if (m_overlayMode == OverlayMode::Search)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::RightButton)
            {
                // Right click anywhere dismisses, mirroring wheel behavior.
                App::getInstance().hideSearchOverlay();
                return true;
            }
            if (mouseEvent->button() == Qt::LeftButton && m_searchPalette != nullptr)
            {
                const QPoint globalPos = mouseEvent->globalPosition().toPoint();
                const QPoint paletteLocal = m_searchPalette->mapFromGlobal(globalPos);
                if (!m_searchPalette->rect().contains(paletteLocal))
                {
                    // Left click outside the palette panel dismisses.
                    App::getInstance().hideSearchOverlay();
                    return true;
                }
            }
        }
        else if (event->type() == QEvent::WindowDeactivate && watched == this)
        {
            // Alt-tab away closes the palette; the user picked a new focus
            // target themselves, so do not fight it by restoring priors.
            App::getInstance().hideSearchOverlay(/*restoreFocus=*/false);
        }
        return QWidget::eventFilter(watched, event);
    }

    if (m_overlayMode != OverlayMode::Wheel)
    {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseMove || event->type() == QEvent::HoverMove)
    {
        m_radialMenu->updateSelectionFromGlobalMousePosition(QCursor::pos());
    }

    if (event->type() == QEvent::MouseButtonPress)
    {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QPoint globalPos = mouseEvent->globalPosition().toPoint();
        const QPoint localPos = mapFromGlobal(globalPos);
        QWidget *clickedChild = childAt(localPos);

        if (mouseEvent->button() == Qt::LeftButton)
        {
            if (clickedChild == m_settingsButton)
            {
                return QWidget::eventFilter(watched, event);
            }

            if (qobject_cast<QAbstractButton *>(watched) != nullptr
                || qobject_cast<QAbstractButton *>(clickedChild) != nullptr)
            {
                return QWidget::eventFilter(watched, event);
            }

            // Any left click on the fullscreen overlay runs the currently selected action.
            m_radialMenu->updateSelectionFromGlobalMousePosition(globalPos);
            App::getInstance().executeAction(m_radialMenu->getSelectedIndex());
            return true;
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
    // Settings is dismissed with Close, not Escape (Escape cancels hotkey
    // recording inside the editor and must not tear the overlay down).
    if (event->key() == Qt::Key_Escape && m_overlayMode != OverlayMode::Settings)
    {
        emit escapePressed();
    }
    else
    {
        QWidget::keyPressEvent(event);
    }
}

void Gui::setMenu(const Menu &menu, const std::vector<ActionSlotVisual> &slotVisuals)
{
    m_titleLabel->setText(QString::fromStdString(menu.getName()));
    m_radialMenu->setMenu(menu, slotVisuals);
}

void Gui::enterInteractiveOverlay()
{
    m_overlayMode = OverlayMode::Wheel;
    if (m_settingsHost != nullptr)
    {
        m_settingsHost->hide();
    }
    if (m_searchHost != nullptr)
    {
        m_searchHost->hide();
    }
    if (m_overlayPanel != nullptr)
    {
        m_overlayPanel->show();
    }
    // Seed Distance-mode selection from wherever the cursor already is.
    m_radialMenu->updateSelectionFromGlobalMousePosition(QCursor::pos());
}

void Gui::enterDormantOverlay()
{
    m_overlayMode = OverlayMode::Dormant;
    if (m_overlayPanel != nullptr)
    {
        m_overlayPanel->hide();
    }
    if (m_settingsHost != nullptr)
    {
        m_settingsHost->hide();
    }
    if (m_searchHost != nullptr)
    {
        m_searchHost->hide();
    }
}

void Gui::showSettingsPanel(SettingsWindow *settingsWindow)
{
    if (settingsWindow == nullptr || m_settingsHost == nullptr || m_settingsHostLayout == nullptr)
    {
        return;
    }

    m_overlayMode = OverlayMode::Settings;
    if (m_overlayPanel != nullptr)
    {
        m_overlayPanel->hide();
    }
    if (m_searchHost != nullptr)
    {
        m_searchHost->hide();
    }

    // A settings editor needs keyboard focus for text boxes and hotkey capture.
    // App switches the native overlay out of no-activate mode before calling
    // this method; wheel mode restores no-activate behavior afterward.
    settingsWindow->setParent(m_settingsHost);
    settingsWindow->setWindowFlags(Qt::Widget);
    settingsWindow->setMaximumSize(1180, 820);
    settingsWindow->setMinimumSize(820, 560);
    settingsWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (m_settingsHostLayout->indexOf(settingsWindow) < 0)
    {
        m_settingsHostLayout->addWidget(settingsWindow, 0, 0, Qt::AlignCenter);
    }

    m_settingsHost->show();
    settingsWindow->show();
    settingsWindow->setFocus();
}

void Gui::hideSettingsPanel()
{
    if (m_overlayMode == OverlayMode::Settings)
    {
        m_overlayMode = OverlayMode::Dormant;
    }
    if (m_settingsHost != nullptr)
    {
        m_settingsHost->hide();
    }
}

void Gui::showSearchPanel(const SearchConfig &config)
{
    if (m_searchHost == nullptr || m_searchPalette == nullptr)
    {
        return;
    }

    m_overlayMode = OverlayMode::Search;
    if (m_overlayPanel != nullptr)
    {
        m_overlayPanel->hide();
    }
    if (m_settingsHost != nullptr)
    {
        m_settingsHost->hide();
    }

    m_searchPalette->openWithConfig(config);
    m_searchHost->show();
}

void Gui::hideSearchPanel()
{
    if (m_overlayMode == OverlayMode::Search)
    {
        m_overlayMode = OverlayMode::Dormant;
    }
    if (m_searchHost != nullptr)
    {
        m_searchHost->hide();
    }
}

void Gui::focusSearchInput()
{
    if (m_searchPalette != nullptr)
    {
        m_searchPalette->focusInput();
    }
}

void Gui::preloadSearchIndex()
{
    if (m_searchPalette != nullptr)
    {
        m_searchPalette->preloadIndex();
    }
}

bool Gui::isLauncherVisible() const
{
    return m_overlayMode == OverlayMode::Wheel;
}

bool Gui::isSettingsVisible() const
{
    return m_overlayMode == OverlayMode::Settings;
}

bool Gui::isSearchVisible() const
{
    return m_overlayMode == OverlayMode::Search;
}

void Gui::refreshSelectionFromCursor()
{
    if (m_radialMenu != nullptr)
    {
        m_radialMenu->updateSelectionFromGlobalMousePosition(QCursor::pos());
    }
}

int Gui::selectedActionIndex() const
{
    if (m_radialMenu == nullptr)
    {
        return -1;
    }
    return m_radialMenu->getSelectedIndex();
}
