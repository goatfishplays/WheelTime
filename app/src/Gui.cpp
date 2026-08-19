#include "App/Gui.hpp"

// TODO: Split Gui into overlay shell vs launcher/settings/search presentation helpers.

#include "App/Action.hpp"
#include "App/App.hpp"
#include "App/SearchPaletteWidget.hpp"
#include "App/SettingsWindow.hpp"

#include <QAbstractButton>
#include <QApplication>
#include <QCursor>
#include <QDebug>
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

    // Ring radius tracks the overlay (monitor) short side.
    m_radialMenu->setActivationMode(RadialMenuWidget::ActivationMode::Distance);

    connect(m_radialMenu, &RadialMenuWidget::selectedIndexChanged, this, &Gui::onSelectChange);
    connect(m_settingsButton, &QPushButton::clicked, []()
            { App::instance().showSettingsWindow(); });
    connect(m_radialMenu, &RadialMenuWidget::buttonTriggered, this, [](int index)
            { App::instance().executeAction(index); });

    m_virtualCursor = new QWidget(this);
    m_virtualCursor->setObjectName("virtualCursor");
    m_virtualCursor->setFixedSize(16, 16);
    m_virtualCursor->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_virtualCursor->hide();

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

void Gui::onSelectChange(int selectionIndex)
{
    qDebug() << "Selected index changed:" << selectionIndex;
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
                App::instance().hideSearchOverlay();
                return true;
            }
            if (mouseEvent->button() == Qt::LeftButton && m_searchPalette != nullptr)
            {
                const QPoint globalPos = mouseEvent->globalPosition().toPoint();
                const QPoint paletteLocal = m_searchPalette->mapFromGlobal(globalPos);
                if (!m_searchPalette->rect().contains(paletteLocal))
                {
                    // Left click outside the palette panel dismisses.
                    App::instance().hideSearchOverlay();
                    return true;
                }
            }
        }
        else if (event->type() == QEvent::WindowDeactivate && watched == this)
        {
            // Alt-tab away closes the palette; the user picked a new focus
            // target themselves, so do not fight it by restoring priors.
            App::instance().hideSearchOverlay(/*restoreFocus=*/false);
        }
        return QWidget::eventFilter(watched, event);
    }

    if (m_overlayMode != OverlayMode::Wheel)
    {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseMove || event->type() == QEvent::HoverMove)
    {
        const QPoint pos = m_virtualCursorOverride.value_or(QCursor::pos());
        m_radialMenu->updateSelectionFromGlobalMousePosition(pos);
    }

    if (event->type() == QEvent::MouseButtonPress)
    {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QPoint globalPos = m_virtualCursorOverride.value_or(mouseEvent->globalPosition().toPoint());
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
            App::instance().executeAction(m_radialMenu->selectedIndex());
            return true;
        }

        if (mouseEvent->button() == Qt::RightButton)
        {
            App::instance().hideGui();
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
    m_titleLabel->setText(QString::fromStdString(menu.name()));
    m_radialMenu->setMenu(menu, slotVisuals);
}

void Gui::applyRice(const RiceSettings &rice)
{
    m_radialMenu->applyRice(rice);
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
    refreshSelectionFromCursor();
}

void Gui::enterDormantOverlay()
{
    m_overlayMode = OverlayMode::Dormant;
    setVirtualCursor(std::nullopt);
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
    settingsWindow->setMinimumSize(960, 680);
    settingsWindow->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    settingsWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (m_settingsHostLayout->indexOf(settingsWindow) < 0)
    {
        m_settingsHostLayout->addWidget(settingsWindow, 0, 0);
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
    if (m_radialMenu == nullptr)
    {
        return;
    }
    const QPoint pos = m_virtualCursorOverride.value_or(QCursor::pos());
    m_radialMenu->updateSelectionFromGlobalMousePosition(pos);
}

void Gui::setVirtualCursor(const std::optional<QPoint> &globalPos)
{
    m_virtualCursorOverride = globalPos;
    if (m_virtualCursor == nullptr)
    {
        return;
    }

    if (!globalPos.has_value())
    {
        m_virtualCursor->hide();
        return;
    }

    m_virtualCursor->show();
    m_virtualCursor->raise();
    const QPoint local = mapFromGlobal(*globalPos);
    m_virtualCursor->move(local.x() - m_virtualCursor->width() / 2,
                          local.y() - m_virtualCursor->height() / 2);
    refreshSelectionFromCursor();
}

bool Gui::hitsSettingsButton(const QPoint &globalPos) const
{
    if (m_settingsButton == nullptr || !m_settingsButton->isVisible())
    {
        return false;
    }
    return m_settingsButton->rect().contains(m_settingsButton->mapFromGlobal(globalPos));
}

QPoint Gui::mouseOpenGlobalPosition(double xFraction, double yFraction) const
{
    if (m_radialMenu == nullptr)
    {
        return QCursor::pos();
    }
    return m_radialMenu->mouseOpenGlobalPosition(xFraction, yFraction);
}

int Gui::selectedActionIndex() const
{
    if (m_radialMenu == nullptr)
    {
        return -1;
    }
    return m_radialMenu->selectedIndex();
}
