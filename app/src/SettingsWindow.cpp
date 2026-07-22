#include "App/SettingsWindow.hpp"

#include "App/ActionItems.hpp"

#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSplitter>
#include <QVBoxLayout>

#include <algorithm>

using namespace Application;

namespace
{
    struct LaunchPresetOption
    {
        const char *id;
        const char *label;
        const char *target;
    };

    constexpr LaunchPresetOption kLaunchPresetOptions[] = {
        {"browser", "Default Browser", "https://www.google.com"},
        {"explorer", "File Explorer", "explorer.exe"},
        {"calculator", "Calculator", "calc.exe"},
        {"notepad", "Notepad", "notepad.exe"},
        {"paint", "Paint", "mspaint.exe"},
        {"taskmgr", "Task Manager", "taskmgr.exe"},
        {"custom", "Custom App...", ""}};

    int modifierMask(bool ctrl, bool alt, bool shift, bool win)
    {
        int mask = 0;
        if (ctrl)
            mask |= 0x0002;
        if (alt)
            mask |= 0x0001;
        if (shift)
            mask |= 0x0004;
        if (win)
            mask |= 0x0008;
        return mask;
    }

    QString slugify(const QString &seed, const QString &prefix)
    {
        QString slug = seed.trimmed().toLower();
        for (int i = 0; i < slug.size(); ++i)
        {
            const QChar c = slug[i];
            if (c.isLetterOrNumber())
            {
                continue;
            }
            slug[i] = '-';
        }

        while (slug.contains("--"))
        {
            slug.replace("--", "-");
        }

        slug.remove(QRegularExpression("^-+"));
        slug.remove(QRegularExpression("-+$"));
        if (slug.isEmpty())
        {
            slug = prefix;
        }

        return prefix + "-" + slug;
    }

    /**
     * @brief Wraps editor content in a transparent, vertical-only scroll area.
     *
     * The editor pages own their styled background; the scroll area must stay
     * invisible so the rounded pane corners are not covered by an opaque
     * rectangle.
     */
    QScrollArea *wrapInScrollArea(QWidget *content, QWidget *parent)
    {
        auto *scroll = new QScrollArea(parent);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setAutoFillBackground(false);
        scroll->viewport()->setAutoFillBackground(false);
        scroll->setWidget(content);
        // Must come after setWidget: QScrollArea::setWidget force-enables
        // autoFillBackground, which paints an opaque palette-colored (dark in
        // dark mode) rectangle behind the sections.
        content->setAutoFillBackground(false);
        return scroll;
    }
}

SettingsWindow::SettingsWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("WheelTime Settings");
    resize(1000, 620);
    setObjectName("settingsWindow");
    // QWidget subclasses need this for QSS background/border painting;
    // without it the styled rounded panel is never drawn and the dark
    // overlay tint shows through between the group boxes.
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_QuitOnClose, false);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    root->addWidget(splitter, 1);

    auto *leftPane = new QWidget(splitter);
    leftPane->setObjectName("settingsSidebar");
    auto *leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(12, 12, 12, 12);
    leftLayout->setSpacing(12);

    m_globalGroup = new QGroupBox("Global Settings", leftPane);
    auto *globalLayout = new QVBoxLayout(m_globalGroup);
    m_darkModeCheck = new QCheckBox("Dark Mode", m_globalGroup);
    m_darkModeCheck->setObjectName("darkModeToggle");
    globalLayout->addWidget(m_darkModeCheck);
    leftLayout->addWidget(m_globalGroup);

    auto *menusGroup = new QGroupBox("Menus", leftPane);
    auto *menusLayout = new QVBoxLayout(menusGroup);
    m_menuList = new QListWidget(menusGroup);
    m_menuList->setObjectName("menuList");
    menusLayout->addWidget(m_menuList);

    auto *menuButtons = new QHBoxLayout();
    auto *addMenuButton = new QPushButton("Add Menu", menusGroup);
    auto *removeMenuButton = new QPushButton("Delete Menu", menusGroup);
    menuButtons->addWidget(addMenuButton);
    menuButtons->addWidget(removeMenuButton);
    menusLayout->addLayout(menuButtons);
    leftLayout->addWidget(menusGroup);

    auto *actionsGroup = new QGroupBox("Actions", leftPane);
    auto *actionsLayout = new QVBoxLayout(actionsGroup);
    m_actionList = new QListWidget(actionsGroup);
    m_actionList->setObjectName("actionList");
    actionsLayout->addWidget(m_actionList);

    auto *actionButtons = new QHBoxLayout();
    auto *addActionButton = new QPushButton("Add Action", actionsGroup);
    auto *removeActionButton = new QPushButton("Delete Action", actionsGroup);
    actionButtons->addWidget(addActionButton);
    actionButtons->addWidget(removeActionButton);
    actionsLayout->addLayout(actionButtons);
    leftLayout->addWidget(actionsGroup);

    m_editorStack = new QStackedWidget(splitter);
    m_editorStack->setObjectName("settingsEditorStack");

    m_emptyEditor = new QWidget(m_editorStack);
    m_emptyEditor->setObjectName("emptyEditor");
    auto *emptyLayout = new QVBoxLayout(m_emptyEditor);
    emptyLayout->addStretch();
    auto *emptyLabel = new QLabel("Select a menu or action to edit.", m_emptyEditor);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyLabel);
    emptyLayout->addStretch();

    m_menuEditor = new QWidget(m_editorStack);
    m_menuEditor->setObjectName("menuEditor");
    auto *menuEditorLayout = new QVBoxLayout(m_menuEditor);
    menuEditorLayout->setContentsMargins(12, 12, 12, 12);
    menuEditorLayout->setSpacing(12);

    // Vertical splitter lets the user rebalance the sections; the scroll area
    // takes over once the sections hit their minimum heights.
    auto *menuEditorSplit = new QSplitter(Qt::Vertical);
    menuEditorSplit->setChildrenCollapsible(false);

    auto *menuSettingsGroup = new QGroupBox("Menu Settings", m_menuEditor);
    auto *menuForm = new QFormLayout(menuSettingsGroup);
    m_menuNameEdit = new QLineEdit(menuSettingsGroup);
    m_keystrokeRecordButton = new QPushButton("Unassigned", menuSettingsGroup);
    m_keystrokeClearButton = new QPushButton("✕", menuSettingsGroup);
    m_keystrokeClearButton->setFixedWidth(30);
    m_keystrokeClearButton->setCursor(Qt::PointingHandCursor);
    m_keystrokeClearButton->setStyleSheet(
        "QPushButton {"
        "   color: #5f6368;"
        "   background-color: transparent;"
        "   font-family: 'Segoe UI', sans-serif;"
        "   font-size: 14px;"
        "   border: none;"
        "   border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #e81123;"
        "   color: white;"
        "}"
    );

    auto *keystrokeLayout = new QHBoxLayout();
    keystrokeLayout->addWidget(m_keystrokeRecordButton);
    keystrokeLayout->addWidget(m_keystrokeClearButton);
    keystrokeLayout->setContentsMargins(0, 0, 0, 0);

    m_keystrokeRecordButton->installEventFilter(this);
    m_executeOnReleaseCheck = new QCheckBox("Execute on release", menuSettingsGroup);
    m_exitOnActionCheck = new QCheckBox("Exit on action", menuSettingsGroup);
    m_centerMouseOnOpenCheck = new QCheckBox("Center mouse on open", menuSettingsGroup);
    m_restoreMouseOnCloseCheck = new QCheckBox("Restore mouse on close", menuSettingsGroup);
    menuForm->addRow("Menu Name", m_menuNameEdit);
    menuForm->addRow("Trigger Keystroke", keystrokeLayout);
    menuForm->addRow("", m_executeOnReleaseCheck);
    menuForm->addRow("", m_exitOnActionCheck);
    menuForm->addRow("", m_centerMouseOnOpenCheck);
    menuForm->addRow("", m_restoreMouseOnCloseCheck);
    menuEditorSplit->addWidget(menuSettingsGroup);

    auto *slotsGroup = new QGroupBox("Wheel Slots", m_menuEditor);
    auto *slotsLayout = new QVBoxLayout(slotsGroup);
    m_slotList = new QListWidget(slotsGroup);
    m_slotList->setObjectName("slotList");
    m_slotList->setMinimumHeight(120);
    slotsLayout->addWidget(m_slotList, 1);

    auto *slotButtons = new QHBoxLayout();
    auto *addSlotButton = new QPushButton("Add Slot", slotsGroup);
    auto *removeSlotButton = new QPushButton("Remove Slot", slotsGroup);
    auto *moveSlotUpButton = new QPushButton("Move Up", slotsGroup);
    auto *moveSlotDownButton = new QPushButton("Move Down", slotsGroup);
    slotButtons->addWidget(addSlotButton);
    slotButtons->addWidget(removeSlotButton);
    slotButtons->addWidget(moveSlotUpButton);
    slotButtons->addWidget(moveSlotDownButton);
    slotsLayout->addLayout(slotButtons);

    m_slotActionCombo = new QComboBox(slotsGroup);
    slotsLayout->addWidget(new QLabel("Assigned Action", slotsGroup));
    slotsLayout->addWidget(m_slotActionCombo);
    menuEditorSplit->addWidget(slotsGroup);
    menuEditorSplit->setStretchFactor(0, 0);
    menuEditorSplit->setStretchFactor(1, 1);
    menuEditorLayout->addWidget(wrapInScrollArea(menuEditorSplit, m_menuEditor));

    m_actionEditor = new QWidget(m_editorStack);
    m_actionEditor->setObjectName("actionEditor");
    auto *actionEditorLayout = new QVBoxLayout(m_actionEditor);
    actionEditorLayout->setContentsMargins(12, 12, 12, 12);
    actionEditorLayout->setSpacing(12);

    auto *actionEditorSplit = new QSplitter(Qt::Vertical);
    actionEditorSplit->setChildrenCollapsible(false);

    auto *actionSettingsGroup = new QGroupBox("Action Settings", m_actionEditor);
    auto *actionForm = new QFormLayout(actionSettingsGroup);
    m_actionNameEdit = new QLineEdit(actionSettingsGroup);
    actionForm->addRow("Action Name", m_actionNameEdit);

    auto *iconRow = new QWidget(actionSettingsGroup);
    auto *iconRowLayout = new QHBoxLayout(iconRow);
    iconRowLayout->setContentsMargins(0, 0, 0, 0);
    m_actionIconEdit = new QLineEdit(iconRow);
    m_actionIconEdit->setPlaceholderText("Optional path to PNG/SVG/ICO...");
    m_browseActionIconButton = new QPushButton("Browse...", iconRow);
    m_clearActionIconButton = new QPushButton("Clear", iconRow);
    iconRowLayout->addWidget(m_actionIconEdit, 1);
    iconRowLayout->addWidget(m_browseActionIconButton);
    iconRowLayout->addWidget(m_clearActionIconButton);
    actionForm->addRow("Icon", iconRow);

    m_actionIconPreview = new QLabel(actionSettingsGroup);
    m_actionIconPreview->setFixedSize(48, 48);
    m_actionIconPreview->setAlignment(Qt::AlignCenter);
    m_actionIconPreview->setStyleSheet("border: 1px solid #adc3d8; border-radius: 6px; background: #ffffff;");
    actionForm->addRow("Preview", m_actionIconPreview);

    m_actionChannelSpin = new QSpinBox(actionSettingsGroup);
    m_actionChannelSpin->setRange(0, 1000000);
    m_actionChannelSpin->setToolTip(
        "0 = independent (does not block other channel-0 actions).\n"
        ">0 = shared FIFO with other actions on the same channel.");
    actionForm->addRow("Run Channel", m_actionChannelSpin);
    m_actionChannelHelpLabel = new QLabel(actionSettingsGroup);
    m_actionChannelHelpLabel->setWordWrap(true);
    m_actionChannelHelpLabel->setStyleSheet("color: #666;");
    actionForm->addRow("", m_actionChannelHelpLabel);
    m_actionSequenceLabel = new QLabel(actionSettingsGroup);
    m_actionSequenceLabel->setWordWrap(true);
    actionForm->addRow("Sequence", m_actionSequenceLabel);
    actionEditorSplit->addWidget(actionSettingsGroup);

    auto *itemsGroup = new QGroupBox("Action Items", m_actionEditor);
    auto *itemsLayout = new QVBoxLayout(itemsGroup);
    m_actionItemList = new QListWidget(itemsGroup);
    m_actionItemList->setObjectName("actionItemList");
    m_actionItemList->setMinimumHeight(140);
    itemsLayout->addWidget(m_actionItemList, 1);

    auto *itemButtons = new QHBoxLayout();
    m_newItemTypeCombo = new QComboBox(itemsGroup);
    // Grouped for scanning: input, timing, navigation, history, cancel, advanced.
    const auto addItemType = [this](const QString &label, const QString &typeId)
    {
        m_newItemTypeCombo->addItem(label, typeId);
    };
    addItemType("Launch App", "launch_app");
    addItemType("Press Keystroke", "keystroke");
    addItemType("Mouse Button", "mouse_button");
    addItemType("Mouse Move", "mouse_move");
    addItemType("Delay", "delay");
    addItemType("Open Menu", "menu");
    addItemType("Search Palette", "search");
    addItemType("Close Launcher", "close");
    addItemType("Nth Recent", "nth_recent");
    addItemType("Nth Frequent", "nth_frequent");
    addItemType("Cancel", "cancel");
    addItemType("Socket Send", "socket");
    addItemType("Custom Script/App (Advanced)", "script");
    auto *addItemButton = new QPushButton("Add Item", itemsGroup);
    auto *removeItemButton = new QPushButton("Remove Item", itemsGroup);
    auto *moveItemUpButton = new QPushButton("Move Up", itemsGroup);
    auto *moveItemDownButton = new QPushButton("Move Down", itemsGroup);
    itemButtons->addWidget(m_newItemTypeCombo);
    itemButtons->addWidget(addItemButton);
    itemButtons->addWidget(removeItemButton);
    itemButtons->addWidget(moveItemUpButton);
    itemButtons->addWidget(moveItemDownButton);
    itemsLayout->addLayout(itemButtons);
    actionEditorSplit->addWidget(itemsGroup);

    m_itemDetailStack = new QStackedWidget(m_actionEditor);
    m_itemDetailStack->setObjectName("itemDetailStack");
    // Floor only; the splitter's surplus height gives the section a roomy
    // default, and the user can shrink it down to this when they want a
    // longer item list.
    m_itemDetailStack->setMinimumHeight(160);
    m_itemNonePage = new QWidget(m_itemDetailStack);
    auto *noneLayout = new QVBoxLayout(m_itemNonePage);
    noneLayout->addWidget(new QLabel("Select an action item to edit its details.", m_itemNonePage));
    noneLayout->addStretch();

    m_itemLaunchAppPage = new QWidget(m_itemDetailStack);
    auto *launchForm = new QFormLayout(m_itemLaunchAppPage);
    m_launchPresetCombo = new QComboBox(m_itemLaunchAppPage);
    populateLaunchPresetCombo();
    m_launchCustomPathEdit = new QLineEdit(m_itemLaunchAppPage);
    m_browseLaunchAppButton = new QPushButton("Browse...", m_itemLaunchAppPage);
    launchForm->addRow("App", m_launchPresetCombo);
    launchForm->addRow("Custom Path", m_launchCustomPathEdit);
    launchForm->addRow("", m_browseLaunchAppButton);

    m_itemScriptPage = new QWidget(m_itemDetailStack);
    auto *scriptForm = new QFormLayout(m_itemScriptPage);
    m_scriptPathEdit = new QLineEdit(m_itemScriptPage);
    m_browseScriptButton = new QPushButton("Browse...", m_itemScriptPage);
    scriptForm->addRow("Path", m_scriptPathEdit);
    scriptForm->addRow("", m_browseScriptButton);

    m_itemDelayPage = new QWidget(m_itemDetailStack);
    auto *delayForm = new QFormLayout(m_itemDelayPage);
    m_delaySpin = new QSpinBox(m_itemDelayPage);
    m_delaySpin->setRange(0, 86400000); // up to 24 hours
    delayForm->addRow("Milliseconds", m_delaySpin);

    m_itemClosePage = new QWidget(m_itemDetailStack);
    auto *closeLayout = new QVBoxLayout(m_itemClosePage);
    closeLayout->addWidget(new QLabel("Close has no extra settings.", m_itemClosePage));
    closeLayout->addStretch();

    m_itemMenuPage = new QWidget(m_itemDetailStack);
    auto *menuTargetForm = new QFormLayout(m_itemMenuPage);
    m_menuTargetCombo = new QComboBox(m_itemMenuPage);
    menuTargetForm->addRow("Target Menu", m_menuTargetCombo);

    m_itemKeystrokePage = new QWidget(m_itemDetailStack);
    auto *keystrokeForm = new QFormLayout(m_itemKeystrokePage);
    auto *keystrokeModifiers = new QWidget(m_itemKeystrokePage);
    auto *keystrokeModifiersLayout = new QHBoxLayout(keystrokeModifiers);
    keystrokeModifiersLayout->setContentsMargins(0, 0, 0, 0);
    keystrokeModifiersLayout->setSpacing(8);
    m_keystrokeCtrlCheck = new QCheckBox("Ctrl", keystrokeModifiers);
    m_keystrokeAltCheck = new QCheckBox("Alt", keystrokeModifiers);
    m_keystrokeShiftCheck = new QCheckBox("Shift", keystrokeModifiers);
    m_keystrokeWinCheck = new QCheckBox("Win", keystrokeModifiers);
    keystrokeModifiersLayout->addWidget(m_keystrokeCtrlCheck);
    keystrokeModifiersLayout->addWidget(m_keystrokeAltCheck);
    keystrokeModifiersLayout->addWidget(m_keystrokeShiftCheck);
    keystrokeModifiersLayout->addWidget(m_keystrokeWinCheck);
    keystrokeModifiersLayout->addStretch();
    m_keystrokeKeyCombo = new QComboBox(m_itemKeystrokePage);
    populateKeystrokeKeyCombo();
    m_keystrokeHoldSpin = new QDoubleSpinBox(m_itemKeystrokePage);
    m_keystrokeHoldSpin->setRange(0.0, 3600.0); // up to 1 hour
    m_keystrokeHoldSpin->setDecimals(2);
    m_keystrokeHoldSpin->setSingleStep(0.1);
    m_keystrokeHoldSpin->setSuffix(" s");
    m_keystrokeProceedCheck = new QCheckBox("Continue immediately", m_itemKeystrokePage);
    keystrokeForm->addRow("Modifiers", keystrokeModifiers);
    keystrokeForm->addRow("Main Key", m_keystrokeKeyCombo);
    keystrokeForm->addRow("Hold Time", m_keystrokeHoldSpin);
    keystrokeForm->addRow("", m_keystrokeProceedCheck);

    m_itemMouseMovePage = new QWidget(m_itemDetailStack);
    auto *mouseMoveForm = new QFormLayout(m_itemMouseMovePage);
    m_mouseMoveXSpin = new QSpinBox(m_itemMouseMovePage);
    m_mouseMoveYSpin = new QSpinBox(m_itemMouseMovePage);
    m_mouseMoveXSpin->setRange(-100000, 100000);
    m_mouseMoveYSpin->setRange(-100000, 100000);
    mouseMoveForm->addRow("X", m_mouseMoveXSpin);
    mouseMoveForm->addRow("Y", m_mouseMoveYSpin);

    m_itemMouseButtonPage = new QWidget(m_itemDetailStack);
    auto *mouseButtonForm = new QFormLayout(m_itemMouseButtonPage);
    auto *mouseButtonModifiers = new QWidget(m_itemMouseButtonPage);
    auto *mouseButtonModifiersLayout = new QHBoxLayout(mouseButtonModifiers);
    mouseButtonModifiersLayout->setContentsMargins(0, 0, 0, 0);
    mouseButtonModifiersLayout->setSpacing(8);
    m_mouseButtonCtrlCheck = new QCheckBox("Ctrl", mouseButtonModifiers);
    m_mouseButtonAltCheck = new QCheckBox("Alt", mouseButtonModifiers);
    m_mouseButtonShiftCheck = new QCheckBox("Shift", mouseButtonModifiers);
    m_mouseButtonWinCheck = new QCheckBox("Win", mouseButtonModifiers);
    mouseButtonModifiersLayout->addWidget(m_mouseButtonCtrlCheck);
    mouseButtonModifiersLayout->addWidget(m_mouseButtonAltCheck);
    mouseButtonModifiersLayout->addWidget(m_mouseButtonShiftCheck);
    mouseButtonModifiersLayout->addWidget(m_mouseButtonWinCheck);
    mouseButtonModifiersLayout->addStretch();
    m_mouseButtonCombo = new QComboBox(m_itemMouseButtonPage);
    m_mouseButtonCombo->addItem("Left", 0);
    m_mouseButtonCombo->addItem("Right", 1);
    m_mouseButtonCombo->addItem("Middle", 2);
    m_mouseButtonHoldSpin = new QDoubleSpinBox(m_itemMouseButtonPage);
    m_mouseButtonHoldSpin->setRange(0.0, 3600.0); // up to 1 hour
    m_mouseButtonHoldSpin->setDecimals(2);
    m_mouseButtonHoldSpin->setSingleStep(0.1);
    m_mouseButtonHoldSpin->setSuffix(" s");
    m_mouseButtonProceedCheck = new QCheckBox("Continue immediately", m_itemMouseButtonPage);
    mouseButtonForm->addRow("Modifiers", mouseButtonModifiers);
    mouseButtonForm->addRow("Button", m_mouseButtonCombo);
    mouseButtonForm->addRow("Hold Time", m_mouseButtonHoldSpin);
    mouseButtonForm->addRow("", m_mouseButtonProceedCheck);

    m_itemCancelPage = new QWidget(m_itemDetailStack);
    auto *cancelForm = new QFormLayout(m_itemCancelPage);
    m_cancelLevelCombo = new QComboBox(m_itemCancelPage);
    m_cancelLevelCombo->addItem("Most Recent", static_cast<int>(CancelLevel::MostRecent));
    m_cancelLevelCombo->addItem("Channel", static_cast<int>(CancelLevel::Channel));
    m_cancelLevelCombo->addItem("All", static_cast<int>(CancelLevel::All));
    m_cancelChannelSpin = new QSpinBox(m_itemCancelPage);
    m_cancelChannelSpin->setRange(0, 1000000);
    m_cancelHelpLabel = new QLabel(m_itemCancelPage);
    m_cancelHelpLabel->setWordWrap(true);
    cancelForm->addRow("Level", m_cancelLevelCombo);
    cancelForm->addRow("Channel", m_cancelChannelSpin);
    cancelForm->addRow(m_cancelHelpLabel);

    m_itemNthPage = new QWidget(m_itemDetailStack);
    auto *nthForm = new QFormLayout(m_itemNthPage);
    m_nthSpin = new QSpinBox(m_itemNthPage);
    m_nthSpin->setRange(1, 1000);
    m_nthHelpLabel = new QLabel(m_itemNthPage);
    m_nthHelpLabel->setWordWrap(true);
    m_resetFrequenciesButton = new QPushButton("Reset all frequencies", m_itemNthPage);
    m_resetFrequenciesButton->setToolTip(
        "Clears launch counts used by Nth Frequent ranking. Recent history is kept.");
    m_resetFrequenciesButton->hide();
    nthForm->addRow("N (1 = top)", m_nthSpin);
    nthForm->addRow(m_nthHelpLabel);
    nthForm->addRow(m_resetFrequenciesButton);

    m_itemSocketPage = new QWidget(m_itemDetailStack);
    auto *socketForm = new QFormLayout(m_itemSocketPage);
    m_socketProtocolCombo = new QComboBox(m_itemSocketPage);
    m_socketProtocolCombo->addItem("UDP", static_cast<int>(Platform::SocketProtocol::Udp));
    m_socketProtocolCombo->addItem("TCP", static_cast<int>(Platform::SocketProtocol::Tcp));
    m_socketProtocolCombo->addItem("HTTP", static_cast<int>(Platform::SocketProtocol::Http));
    m_socketProtocolCombo->addItem("WebSocket", static_cast<int>(Platform::SocketProtocol::WebSocket));
    m_socketDestinationEdit = new QLineEdit(m_itemSocketPage);
    m_socketDestinationEdit->setPlaceholderText("host:port or URL");
    m_socketMessageEdit = new QLineEdit(m_itemSocketPage);
    m_socketMessageEdit->setPlaceholderText("payload / body");
    m_socketHttpMethodEdit = new QLineEdit(m_itemSocketPage);
    m_socketHttpMethodEdit->setPlaceholderText("POST");
    m_socketHttpMethodEdit->setText("POST");
    m_socketHelpLabel = new QLabel(m_itemSocketPage);
    m_socketHelpLabel->setWordWrap(true);
    m_socketHelpLabel->setText("UDP/TCP use host:port. HTTP/WebSocket use a full URL.");
    socketForm->addRow("Protocol", m_socketProtocolCombo);
    socketForm->addRow("Destination", m_socketDestinationEdit);
    socketForm->addRow("Message", m_socketMessageEdit);
    socketForm->addRow("HTTP Method", m_socketHttpMethodEdit);
    socketForm->addRow(m_socketHelpLabel);

    m_itemSearchPage = new QWidget(m_itemDetailStack);
    auto *searchForm = new QFormLayout(m_itemSearchPage);
    m_searchActionsCheck = new QCheckBox("Search actions", m_itemSearchPage);
    m_searchProgramsCheck = new QCheckBox("Search programs", m_itemSearchPage);
    m_searchMenusCheck = new QCheckBox("Search menus", m_itemSearchPage);
    m_searchWebCheck = new QCheckBox("Web search fallback", m_itemSearchPage);
    m_searchWebUrlEdit = new QLineEdit(m_itemSearchPage);
    m_searchWebUrlEdit->setPlaceholderText("https://www.google.com/search?q={query}");
    m_searchHelpLabel = new QLabel(m_itemSearchPage);
    m_searchHelpLabel->setWordWrap(true);
    m_searchHelpLabel->setText(
        "Opens the search palette with these filters enabled (still toggleable "
        "in the palette). {query} in the URL is replaced with the search text.");
    searchForm->addRow("Filters", m_searchActionsCheck);
    searchForm->addRow("", m_searchProgramsCheck);
    searchForm->addRow("", m_searchMenusCheck);
    searchForm->addRow("", m_searchWebCheck);
    searchForm->addRow("Web Search URL", m_searchWebUrlEdit);
    searchForm->addRow(m_searchHelpLabel);

    m_itemKeyReleasePage = new QWidget(m_itemDetailStack);
    auto *keyReleaseLayout = new QVBoxLayout(m_itemKeyReleasePage);
    m_keyReleaseHelpLabel = new QLabel(m_itemKeyReleasePage);
    m_keyReleaseHelpLabel->setWordWrap(true);
    keyReleaseLayout->addWidget(m_keyReleaseHelpLabel);
    keyReleaseLayout->addStretch();

    m_itemDetailStack->addWidget(m_itemNonePage);
    m_itemDetailStack->addWidget(m_itemLaunchAppPage);
    m_itemDetailStack->addWidget(m_itemScriptPage);
    m_itemDetailStack->addWidget(m_itemDelayPage);
    m_itemDetailStack->addWidget(m_itemClosePage);
    m_itemDetailStack->addWidget(m_itemMenuPage);
    m_itemDetailStack->addWidget(m_itemKeystrokePage);
    m_itemDetailStack->addWidget(m_itemMouseMovePage);
    m_itemDetailStack->addWidget(m_itemMouseButtonPage);
    m_itemDetailStack->addWidget(m_itemCancelPage);
    m_itemDetailStack->addWidget(m_itemNthPage);
    m_itemDetailStack->addWidget(m_itemSocketPage);
    m_itemDetailStack->addWidget(m_itemSearchPage);
    m_itemDetailStack->addWidget(m_itemKeyReleasePage);
    auto *detailGroup = new QGroupBox("Item Details", m_actionEditor);
    auto *detailLayout = new QVBoxLayout(detailGroup);
    detailLayout->addWidget(m_itemDetailStack);
    actionEditorSplit->addWidget(detailGroup);
    actionEditorSplit->setStretchFactor(0, 0);
    actionEditorSplit->setStretchFactor(1, 1);
    actionEditorSplit->setStretchFactor(2, 1);
    // Guarantee drag slack: keep the splitter taller than its sections'
    // combined minimums, otherwise every section pins at its minimum and the
    // handles cannot move. The scroll area absorbs the surplus height.
    actionEditorSplit->setMinimumHeight(
        actionEditorSplit->minimumSizeHint().height() + 240);
    actionEditorLayout->addWidget(wrapInScrollArea(actionEditorSplit, m_actionEditor));

    m_editorStack->addWidget(m_emptyEditor);
    m_editorStack->addWidget(m_menuEditor);
    m_editorStack->addWidget(m_actionEditor);

    auto *bottomButtons = new QHBoxLayout();
    bottomButtons->addStretch();
    auto *saveButton = new QPushButton("Save", this);
    auto *closeButton = new QPushButton("Close", this);
    saveButton->setObjectName("primaryButton");
    bottomButtons->addWidget(saveButton);
    bottomButtons->addWidget(closeButton);
    root->addLayout(bottomButtons);

    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
    connect(saveButton, &QPushButton::clicked, this, [this]()
            {
                QString errorMessage;
                if (!validateWorkingCopy(errorMessage))
                {
                    QMessageBox::warning(this, "Invalid Configuration", errorMessage);
                    return;
                }
                emit saveRequested(); 
                if (m_darkModeCheck) {
                    m_darkModeCheck->setText(m_darkModeCheck->isChecked() ? "Light Mode" : "Dark Mode");
                } });

    connect(m_keystrokeRecordButton, &QPushButton::clicked, this, [this]()
            {
                if (!m_isRecordingKeystroke) {
                    m_isRecordingKeystroke = true;
                    m_keystrokeRecordButton->setText("Press any key combination...");
                    m_keystrokeRecordButton->grabKeyboard();
                } });

    connect(m_keystrokeClearButton, &QPushButton::clicked, this, [this]()
            {
                int index = currentMenuIndex();
                if (index >= 0) {
                    m_menus[index].setTriggerMod(0);
                    m_menus[index].setTriggerVk(0);
                    updateKeystrokeButtonText();
                } });

    connect(m_menuList, &QListWidget::currentRowChanged, this, [this](int row)
            {
                if (row >= 0)
                {
                    setSelectedMenu(row);
                } });
    connect(m_menuList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item)
            {
                if (item != nullptr)
                {
                    setSelectedMenu(m_menuList->row(item));
                } });
    connect(m_actionList, &QListWidget::currentRowChanged, this, [this](int row)
            {
                if (row >= 0)
                {
                    setSelectedAction(row);
                } });
    connect(m_actionList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item)
            {
                if (item != nullptr)
                {
                    setSelectedAction(m_actionList->row(item));
                } });

    connect(addMenuButton, &QPushButton::clicked, this, [this]()
            {
                Menu menu(0, 0, false, false, true, false, "New Menu", {}, makeUniqueMenuId("New Menu"));
                m_menus.push_back(menu);
                refreshLists();
                setSelectedMenu(static_cast<int>(m_menus.size()) - 1); });
    connect(removeMenuButton, &QPushButton::clicked, this, [this]()
            {
                const int index = currentMenuIndex();
                if (index < 0)
                {
                    return;
                }
                const std::string removedMenuId = m_menus[index].id();
                m_menus.erase(m_menus.begin() + index);
                clearMenuReferences(removedMenuId);
                refreshLists();
                refreshEditors(); });
    connect(addActionButton, &QPushButton::clicked, this, [this]()
            {
                std::vector<std::unique_ptr<ActionItem>> items;
                items.push_back(std::make_unique<AI_LaunchApp>("calculator", ""));
                m_actions.emplace_back(std::move(items), "New Action", "", makeUniqueActionId("New Action"));
                refreshLists();
                setSelectedAction(static_cast<int>(m_actions.size()) - 1); });
    connect(removeActionButton, &QPushButton::clicked, this, [this]()
            {
                const int index = currentActionIndex();
                if (index < 0)
                {
                    return;
                }
                const std::string removedActionId = m_actions[index].id();
                m_actions.erase(m_actions.begin() + index);
                deleteActionReferences(removedActionId);
                refreshLists();
                refreshEditors(); });

    connect(m_menuNameEdit, &QLineEdit::textEdited, this, [this](const QString &text)
            {
                const int index = currentMenuIndex();
                if (index >= 0)
                {
                    m_menus[index].setName(text.toStdString());
                    refreshMenuList();
                } });
    connect(m_executeOnReleaseCheck, &QCheckBox::toggled, this, [this](bool checked)
            {
                const int index = currentMenuIndex();
                if (index >= 0)
                {
                    m_menus[index].setExecuteOnRelease(checked);
                } });
    connect(m_exitOnActionCheck, &QCheckBox::toggled, this, [this](bool checked)
            {
                const int index = currentMenuIndex();
                if (index >= 0)
                {
                    m_menus[index].setExitOnAction(checked);
                } });
    connect(m_centerMouseOnOpenCheck, &QCheckBox::toggled, this, [this](bool checked)
            {
                const int index = currentMenuIndex();
                if (index >= 0)
                {
                    m_menus[index].setCenterMouseOnOpen(checked);
                } });
    connect(m_restoreMouseOnCloseCheck, &QCheckBox::toggled, this, [this](bool checked)
            {
                const int index = currentMenuIndex();
                if (index >= 0)
                {
                    m_menus[index].setRestoreMouseOnClose(checked);
                } });
    connect(addSlotButton, &QPushButton::clicked, this, [this]()
            {
                const int menuIndex = currentMenuIndex();
                if (menuIndex < 0)
                {
                    return;
                }
                if (m_actions.empty())
                {
                    QMessageBox::information(this, "No Actions", "Create an action first, then assign it to a menu slot.");
                    return;
                }
                m_menus[menuIndex].addActionId(-1, m_actions.front().id());
                refreshMenuEditor(); });
    connect(removeSlotButton, &QPushButton::clicked, this, [this]()
            {
                const int menuIndex = currentMenuIndex();
                const int slotIndex = m_slotList->currentRow();
                if (menuIndex >= 0 && slotIndex >= 0)
                {
                    m_menus[menuIndex].removeAction(slotIndex);
                    refreshMenuEditor();
                } });
    connect(moveSlotUpButton, &QPushButton::clicked, this, [this]()
            {
                const int menuIndex = currentMenuIndex();
                const int slotIndex = m_slotList->currentRow();
                if (menuIndex >= 0 && slotIndex > 0)
                {
                    m_menus[menuIndex].moveActionId(slotIndex, slotIndex - 1);
                    refreshMenuEditor();
                    m_slotList->setCurrentRow(slotIndex - 1);
                } });
    connect(moveSlotDownButton, &QPushButton::clicked, this, [this]()
            {
                const int menuIndex = currentMenuIndex();
                const int slotIndex = m_slotList->currentRow();
                if (menuIndex >= 0 && slotIndex >= 0 && slotIndex + 1 < m_menus[menuIndex].actionCount())
                {
                    m_menus[menuIndex].moveActionId(slotIndex, slotIndex + 1);
                    refreshMenuEditor();
                    m_slotList->setCurrentRow(slotIndex + 1);
                } });
    connect(m_slotList, &QListWidget::currentRowChanged, this, [this](int)
            {
                refreshSlotActionCombo(); });
    connect(m_slotActionCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int comboIndex)
            {
                if (m_isRefreshing)
                {
                    return;
                }
                const int menuIndex = currentMenuIndex();
                const int slotIndex = m_slotList->currentRow();
                if (menuIndex < 0 || slotIndex < 0 || comboIndex < 0)
                {
                    return;
                }
                const QString actionId = m_slotActionCombo->itemData(comboIndex).toString();
                m_menus[menuIndex].setActionId(slotIndex, actionId.toStdString());
                refreshSlotList(); });

    connect(m_actionNameEdit, &QLineEdit::textEdited, this, [this](const QString &text)
            {
                const int index = currentActionIndex();
                if (index >= 0)
                {
                    m_actions[index].setName(text.toStdString());
                    refreshActionList();
                } });
    connect(m_actionIconEdit, &QLineEdit::textEdited, this, [this](const QString &text)
            {
                const int index = currentActionIndex();
                if (index >= 0)
                {
                    m_actions[index].setIconFilepath(text.toStdString());
                    refreshActionIconPreview();
                } });
    connect(m_browseActionIconButton, &QPushButton::clicked, this, [this]()
            {
                const int index = currentActionIndex();
                if (index < 0)
                {
                    return;
                }

                const QString selectedPath = QFileDialog::getOpenFileName(
                    this,
                    "Choose Action Icon",
                    QString::fromStdString(m_actions[index].iconFilepath()),
                    "Images (*.png *.jpg *.jpeg *.bmp *.gif *.svg *.ico);;All Files (*.*)");
                if (selectedPath.isEmpty())
                {
                    return;
                }

                m_actions[index].setIconFilepath(selectedPath.toStdString());
                m_actionIconEdit->setText(selectedPath);
                refreshActionIconPreview(); });
    connect(m_clearActionIconButton, &QPushButton::clicked, this, [this]()
            {
                const int index = currentActionIndex();
                if (index < 0)
                {
                    return;
                }
                m_actions[index].setIconFilepath("");
                m_actionIconEdit->clear();
                refreshActionIconPreview(); });
    connect(m_actionChannelSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value)
            {
                const int index = currentActionIndex();
                if (index < 0)
                {
                    return;
                }
                m_actions[index].setChannel(static_cast<uint32_t>(value));
                if (value == 0)
                {
                    m_actionChannelHelpLabel->setText(
                        "Independent: runs without blocking other channel-0 actions.");
                }
                else
                {
                    m_actionChannelHelpLabel->setText(
                        QString("Shared FIFO: only one action on channel %1 runs at a time.").arg(value));
                }
                refreshActionList();
            });
    connect(addItemButton, &QPushButton::clicked, this, [this]()
            {
                const int actionIndex = currentActionIndex();
                if (actionIndex < 0)
                {
                    return;
                }

                std::unique_ptr<ActionItem> item;
                const QString typeId = m_newItemTypeCombo->currentData().toString();
                if (typeId == "launch_app")
                {
                    item = std::make_unique<AI_LaunchApp>("calculator", "");
                }
                else if (typeId == "keystroke")
                {
                    item = std::make_unique<AI_Keystroke>('A', 0, 0.0f, false);
                }
                else if (typeId == "mouse_button")
                {
                    item = std::make_unique<AI_MouseButton>(0, 0.0f, false);
                }
                else if (typeId == "mouse_move")
                {
                    item = std::make_unique<AI_MouseMove>(0, 0);
                }
                else if (typeId == "delay")
                {
                    item = std::make_unique<AI_Delay>(0);
                }
                else if (typeId == "menu")
                {
                    item = std::make_unique<AI_Menu>(m_menus.empty() ? "" : m_menus.front().id());
                }
                else if (typeId == "search")
                {
                    item = std::make_unique<AI_Search>();
                }
                else if (typeId == "close")
                {
                    item = std::make_unique<AI_Close>();
                }
                else if (typeId == "nth_recent")
                {
                    item = std::make_unique<AI_NthRecent>(1);
                }
                else if (typeId == "nth_frequent")
                {
                    item = std::make_unique<AI_NthFrequent>(1);
                }
                else if (typeId == "cancel")
                {
                    item = std::make_unique<AI_Cancel>(CancelLevel::MostRecent, 0);
                }
                else if (typeId == "socket")
                {
                    item = std::make_unique<AI_Socket>(
                        Platform::SocketProtocol::Udp, "127.0.0.1:9000", "", "POST");
                }
                else if (typeId == "script")
                {
                    item = std::make_unique<AI_Script>("");
                }
                else
                {
                    return;
                }
                m_actions[actionIndex].addItem(-1, std::move(item));
                refreshActionEditor();
                m_actionItemList->setCurrentRow(m_actions[actionIndex].itemCount() - 1); });
    connect(removeItemButton, &QPushButton::clicked, this, [this]()
            {
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                if (actionIndex >= 0 && itemIndex >= 0)
                {
                    m_actions[actionIndex].removeItem(itemIndex);
                    refreshActionEditor();
                } });
    connect(moveItemUpButton, &QPushButton::clicked, this, [this]()
            {
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                if (actionIndex >= 0 && itemIndex > 0)
                {
                    m_actions[actionIndex].moveItem(itemIndex, itemIndex - 1);
                    refreshActionEditor();
                    m_actionItemList->setCurrentRow(itemIndex - 1);
                } });
    connect(moveItemDownButton, &QPushButton::clicked, this, [this]()
            {
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                if (actionIndex >= 0 && itemIndex >= 0 && itemIndex + 1 < m_actions[actionIndex].itemCount())
                {
                    m_actions[actionIndex].moveItem(itemIndex, itemIndex + 1);
                    refreshActionEditor();
                    m_actionItemList->setCurrentRow(itemIndex + 1);
                } });
    connect(m_actionItemList, &QListWidget::currentRowChanged, this, [this](int)
            {
                refreshItemDetail(); });
    connect(m_launchPresetCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int comboIndex)
            {
                if (m_isRefreshing)
                {
                    return;
                }
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_LaunchApp *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
                if (item == nullptr || comboIndex < 0)
                {
                    return;
                }

                item->presetId = m_launchPresetCombo->itemData(comboIndex).toString().toStdString();
                if (item->presetId != "custom")
                {
                    item->customTarget = launchPresetTarget(item->presetId).toStdString();
                }
                refreshActionItemList();
                refreshActionSummary();
                refreshItemDetail(); });
    connect(m_launchCustomPathEdit, &QLineEdit::textEdited, this, [this](const QString &text)
            {
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_LaunchApp *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
                if (item != nullptr)
                {
                    item->customTarget = text.toStdString();
                    refreshActionItemList();
                    refreshActionSummary();
                } });
    connect(m_browseLaunchAppButton, &QPushButton::clicked, this, [this]()
            {
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_LaunchApp *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
                if (item == nullptr)
                {
                    return;
                }

                const QString selectedPath = QFileDialog::getOpenFileName(
                    this,
                    "Choose App or Script",
                    QString::fromStdString(item->customTarget),
                    "Apps and Scripts (*.exe *.bat *.cmd *.ps1 *.py *.lnk);;All Files (*.*)");
                if (selectedPath.isEmpty())
                {
                    return;
                }

                item->presetId = "custom";
                item->customTarget = selectedPath.toStdString();
                refreshActionItemList();
                refreshActionSummary();
                refreshItemDetail(); });
    connect(m_scriptPathEdit, &QLineEdit::textEdited, this, [this](const QString &text)
            {
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_Script *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
                if (item != nullptr)
                {
                    item->filepath = text.toStdString();
                    refreshActionItemList();
                    refreshActionSummary();
                } });
    connect(m_browseScriptButton, &QPushButton::clicked, this, [this]()
            {
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_Script *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
                if (item == nullptr)
                {
                    return;
                }

                const QString selectedPath = QFileDialog::getOpenFileName(
                    this,
                    "Choose App or Script",
                    QString::fromStdString(item->filepath),
                    "Apps and Scripts (*.exe *.bat *.cmd *.ps1 *.py *.lnk);;All Files (*.*)");
                if (selectedPath.isEmpty())
                {
                    return;
                }

                item->filepath = selectedPath.toStdString();
                m_scriptPathEdit->setText(selectedPath);
                refreshActionItemList();
                refreshActionSummary(); });
    auto keystrokeEditorChanged = [this]()
    {
        const int actionIndex = currentActionIndex();
        const int itemIndex = currentActionItemIndex();
        auto *item = actionIndex >= 0 ? dynamic_cast<AI_Keystroke *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
        if (item == nullptr)
        {
            return;
        }

        item->modifiers = modifierMask(
            m_keystrokeCtrlCheck->isChecked(),
            m_keystrokeAltCheck->isChecked(),
            m_keystrokeShiftCheck->isChecked(),
            m_keystrokeWinCheck->isChecked());
        item->keycode = m_keystrokeKeyCombo->currentData().toInt();
        item->holdDurationSec = static_cast<float>(m_keystrokeHoldSpin->value());
        item->proceed = m_keystrokeProceedCheck->isChecked();
        refreshActionItemList();
        refreshActionSummary();
    };
    connect(m_keystrokeCtrlCheck, &QCheckBox::toggled, this, [keystrokeEditorChanged](bool) { keystrokeEditorChanged(); });
    connect(m_keystrokeAltCheck, &QCheckBox::toggled, this, [keystrokeEditorChanged](bool) { keystrokeEditorChanged(); });
    connect(m_keystrokeShiftCheck, &QCheckBox::toggled, this, [keystrokeEditorChanged](bool) { keystrokeEditorChanged(); });
    connect(m_keystrokeWinCheck, &QCheckBox::toggled, this, [keystrokeEditorChanged](bool) { keystrokeEditorChanged(); });
    connect(m_keystrokeKeyCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, keystrokeEditorChanged](int)
            {
                if (!m_isRefreshing)
                {
                    keystrokeEditorChanged();
                } });
    connect(m_keystrokeHoldSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [keystrokeEditorChanged](double) { keystrokeEditorChanged(); });
    connect(m_keystrokeProceedCheck, &QCheckBox::toggled, this, [keystrokeEditorChanged](bool) { keystrokeEditorChanged(); });
    connect(m_mouseMoveXSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value)
            {
                if (m_isRefreshing)
                {
                    return;
                }
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_MouseMove *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
                if (item != nullptr)
                {
                    item->x = value;
                    refreshActionItemList();
                    refreshActionSummary();
                } });
    connect(m_mouseMoveYSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value)
            {
                if (m_isRefreshing)
                {
                    return;
                }
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_MouseMove *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
                if (item != nullptr)
                {
                    item->y = value;
                    refreshActionItemList();
                    refreshActionSummary();
                } });
    auto mouseButtonEditorChanged = [this]()
    {
        if (m_isRefreshing)
        {
            return;
        }
        const int actionIndex = currentActionIndex();
        const int itemIndex = currentActionItemIndex();
        auto *item = actionIndex >= 0 ? dynamic_cast<AI_MouseButton *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
        if (item == nullptr)
        {
            return;
        }
        item->button = m_mouseButtonCombo->currentData().toInt();
        item->modifiers = modifierMask(
            m_mouseButtonCtrlCheck->isChecked(),
            m_mouseButtonAltCheck->isChecked(),
            m_mouseButtonShiftCheck->isChecked(),
            m_mouseButtonWinCheck->isChecked());
        item->holdDurationSec = static_cast<float>(m_mouseButtonHoldSpin->value());
        item->proceed = m_mouseButtonProceedCheck->isChecked();
        refreshActionItemList();
        refreshActionSummary();
    };
    connect(m_mouseButtonCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [mouseButtonEditorChanged](int)
            { mouseButtonEditorChanged(); });
    connect(m_mouseButtonCtrlCheck, &QCheckBox::toggled, this, [mouseButtonEditorChanged](bool)
            { mouseButtonEditorChanged(); });
    connect(m_mouseButtonAltCheck, &QCheckBox::toggled, this, [mouseButtonEditorChanged](bool)
            { mouseButtonEditorChanged(); });
    connect(m_mouseButtonShiftCheck, &QCheckBox::toggled, this, [mouseButtonEditorChanged](bool)
            { mouseButtonEditorChanged(); });
    connect(m_mouseButtonWinCheck, &QCheckBox::toggled, this, [mouseButtonEditorChanged](bool)
            { mouseButtonEditorChanged(); });
    connect(m_mouseButtonHoldSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [mouseButtonEditorChanged](double)
            { mouseButtonEditorChanged(); });
    connect(m_mouseButtonProceedCheck, &QCheckBox::toggled, this, [mouseButtonEditorChanged](bool)
            { mouseButtonEditorChanged(); });
    auto cancelEditorChanged = [this]()
    {
        if (m_isRefreshing)
        {
            return;
        }
        const int actionIndex = currentActionIndex();
        const int itemIndex = currentActionItemIndex();
        auto *item = actionIndex >= 0 ? dynamic_cast<AI_Cancel *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
        if (item == nullptr)
        {
            return;
        }
        item->level = static_cast<CancelLevel>(m_cancelLevelCombo->currentData().toInt());
        item->channel = static_cast<uint32_t>(m_cancelChannelSpin->value());
        updateCancelChannelVisibility(item->level);
        if (item->level == CancelLevel::MostRecent)
        {
            m_cancelHelpLabel->setText(
                "Cancels the most recently submitted action on the chosen channel. "
                "Use channel 0 for the most recent action on any channel. "
                "Never cancels the action that contains this item.");
        }
        else if (item->level == CancelLevel::Channel)
        {
            m_cancelHelpLabel->setText("Cancels every cancelable action on the chosen channel.");
        }
        else
        {
            m_cancelHelpLabel->setText("Cancels every cancelable action.");
        }
        refreshActionItemList();
        refreshActionSummary();
    };
    connect(m_cancelLevelCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [cancelEditorChanged](int)
            { cancelEditorChanged(); });
    connect(m_cancelChannelSpin, qOverload<int>(&QSpinBox::valueChanged), this, [cancelEditorChanged](int)
            { cancelEditorChanged(); });
    connect(m_nthSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value)
            {
                if (m_isRefreshing)
                {
                    return;
                }
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                if (actionIndex < 0 || itemIndex < 0)
                {
                    return;
                }
                if (auto *recent = dynamic_cast<AI_NthRecent *>(m_actions[actionIndex].item(itemIndex)))
                {
                    recent->n = value;
                }
                else if (auto *frequent = dynamic_cast<AI_NthFrequent *>(m_actions[actionIndex].item(itemIndex)))
                {
                    frequent->n = value;
                }
                else
                {
                    return;
                }
                refreshActionItemList();
                refreshActionSummary(); });
    connect(m_resetFrequenciesButton, &QPushButton::clicked, this, [this]()
            {
                const auto answer = QMessageBox::question(
                    this,
                    "Reset Frequencies",
                    "Clear all action launch frequencies? Nth Frequent ranking will start empty. "
                    "Recent history is not affected.",
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);
                if (answer != QMessageBox::Yes)
                {
                    return;
                }
                App::instance().resetActionFrequencies();
                QMessageBox::information(this, "Frequencies Reset",
                                         "All action launch frequencies have been cleared.");
            });
    auto socketEditorChanged = [this]()
    {
        if (m_isRefreshing)
        {
            return;
        }
        const int actionIndex = currentActionIndex();
        const int itemIndex = currentActionItemIndex();
        auto *item = actionIndex >= 0 ? dynamic_cast<AI_Socket *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
        if (item == nullptr)
        {
            return;
        }
        item->protocol = static_cast<Platform::SocketProtocol>(m_socketProtocolCombo->currentData().toInt());
        item->outputDst = m_socketDestinationEdit->text().trimmed().toStdString();
        item->socketMsg = m_socketMessageEdit->text().toStdString();
        item->httpMethod = m_socketHttpMethodEdit->text().trimmed().toStdString();
        if (item->httpMethod.empty())
        {
            item->httpMethod = "POST";
        }
        const bool isHttp = item->protocol == Platform::SocketProtocol::Http;
        m_socketHttpMethodEdit->setEnabled(isHttp);
        refreshActionItemList();
        refreshActionSummary();
    };
    connect(m_socketProtocolCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [socketEditorChanged](int) { socketEditorChanged(); });
    connect(m_socketDestinationEdit, &QLineEdit::textChanged, this, [socketEditorChanged](const QString &)
            { socketEditorChanged(); });
    connect(m_socketMessageEdit, &QLineEdit::textChanged, this, [socketEditorChanged](const QString &)
            { socketEditorChanged(); });
    connect(m_socketHttpMethodEdit, &QLineEdit::textChanged, this, [socketEditorChanged](const QString &)
            { socketEditorChanged(); });
    auto searchEditorChanged = [this]()
    {
        if (m_isRefreshing)
        {
            return;
        }
        const int actionIndex = currentActionIndex();
        const int itemIndex = currentActionItemIndex();
        auto *item = actionIndex >= 0 ? dynamic_cast<AI_Search *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
        if (item == nullptr)
        {
            return;
        }
        item->config.searchActions = m_searchActionsCheck->isChecked();
        item->config.searchPrograms = m_searchProgramsCheck->isChecked();
        item->config.searchMenus = m_searchMenusCheck->isChecked();
        item->config.webSearch = m_searchWebCheck->isChecked();
        std::string url = m_searchWebUrlEdit->text().trimmed().toStdString();
        if (url.empty())
        {
            url = "https://www.google.com/search?q={query}";
        }
        item->config.webSearchUrl = url;
        m_searchWebUrlEdit->setEnabled(item->config.webSearch);
        refreshActionItemList();
        refreshActionSummary();
    };
    connect(m_searchActionsCheck, &QCheckBox::toggled, this, [searchEditorChanged](bool)
            { searchEditorChanged(); });
    connect(m_searchProgramsCheck, &QCheckBox::toggled, this, [searchEditorChanged](bool)
            { searchEditorChanged(); });
    connect(m_searchMenusCheck, &QCheckBox::toggled, this, [searchEditorChanged](bool)
            { searchEditorChanged(); });
    connect(m_searchWebCheck, &QCheckBox::toggled, this, [searchEditorChanged](bool)
            { searchEditorChanged(); });
    connect(m_searchWebUrlEdit, &QLineEdit::textChanged, this, [searchEditorChanged](const QString &)
            { searchEditorChanged(); });
    connect(m_delaySpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value)
            {
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_Delay *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
                if (item != nullptr)
                {
                    item->durationMs = value;
                    refreshActionItemList();
                    refreshActionSummary();
                } });
    connect(m_menuTargetCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int comboIndex)
            {
                if (m_isRefreshing)
                {
                    return;
                }
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_Menu *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
                if (item != nullptr && comboIndex >= 0)
                {
                    item->menuId = m_menuTargetCombo->itemData(comboIndex).toString().toStdString();
                    refreshActionItemList();
                    refreshActionSummary();
                } });

    m_editorStack->setCurrentWidget(m_emptyEditor);
    m_itemDetailStack->setCurrentWidget(m_itemNonePage);
}

void SettingsWindow::updateKeystrokeButtonText()
{
    int idx = currentMenuIndex();
    if (idx < 0)
    {
        return;
    }
    int mod = m_menus[idx].triggerMod();
    int vk = m_menus[idx].triggerVk();

    QString mods;
    if (mod & 0x0002)
    {
        mods += "Ctrl + ";
    }
    if (mod & 0x0008)
    {
        mods += "Win + ";
    }
    if (mod & 0x0001)
    {
        mods += "Alt + ";
    }
    if (mod & 0x0004)
    {
        mods += "Shift + ";
    }

    if (mod == 0 && vk == 0)
    {
        m_keystrokeRecordButton->setText("Unassigned");
    }
    else
    {
        m_keystrokeRecordButton->setText(mods + keyDisplayName(vk));
    }
}

void SettingsWindow::updateCancelChannelVisibility(CancelLevel level)
{
    const bool needsChannel = level == CancelLevel::MostRecent || level == CancelLevel::Channel;
    m_cancelChannelSpin->setVisible(needsChannel);
    if (auto *form = qobject_cast<QFormLayout *>(m_itemCancelPage->layout()))
    {
        if (QWidget *label = form->labelForField(m_cancelChannelSpin))
        {
            label->setVisible(needsChannel);
        }
    }
}

bool SettingsWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (m_isRecordingKeystroke && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        int key = keyEvent->key();
        
        // Ignore modifier-only presses until they press a main key
        if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta || 
            key == Qt::Key_Super_L || key == Qt::Key_Super_R || 
            key == Qt::Key_AltGr || key == Qt::Key_Menu)
        {
            return true;
        }

        // Escape cancels recording without changing the existing bind.
        if (key == Qt::Key_Escape)
        {
            m_isRecordingKeystroke = false;
            m_keystrokeRecordButton->releaseKeyboard();
            updateKeystrokeButtonText();
            return true;
        }

        int idx = currentMenuIndex();
        if (idx >= 0)
        {
            const int newVk = static_cast<int>(keyEvent->nativeVirtualKey());
            int newMod = 0;

            Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
            if (modifiers & Qt::ControlModifier) newMod |= 0x0002;
            if (modifiers & Qt::AltModifier)     newMod |= 0x0001;
            if (modifiers & Qt::ShiftModifier)   newMod |= 0x0004;
            if (modifiers & Qt::MetaModifier)    newMod |= 0x0008;

            QString conflictMenuName;
            for (int i = 0; i < static_cast<int>(m_menus.size()); ++i)
            {
                if (i == idx)
                {
                    continue;
                }
                if (m_menus[i].triggerVk() == newVk && m_menus[i].triggerMod() == newMod && newVk != 0)
                {
                    conflictMenuName = QString::fromStdString(m_menus[i].name());
                    break;
                }
            }

            if (!conflictMenuName.isEmpty())
            {
                QMessageBox::warning(
                    this,
                    "Keystroke Already Used",
                    QString("That keystroke is already assigned to \"%1\". "
                            "Clear the other menu's keystroke first, or choose a different combination.")
                        .arg(conflictMenuName));
                m_isRecordingKeystroke = false;
                m_keystrokeRecordButton->releaseKeyboard();
                updateKeystrokeButtonText();
                return true;
            }

            m_menus[idx].setTriggerVk(newVk);
            m_menus[idx].setTriggerMod(newMod);
        }

        m_isRecordingKeystroke = false;
        m_keystrokeRecordButton->releaseKeyboard();
        updateKeystrokeButtonText();
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void SettingsWindow::closeEvent(QCloseEvent *event)
{
    if (m_isRecordingKeystroke)
    {
        m_isRecordingKeystroke = false;
        if (m_keystrokeRecordButton != nullptr)
        {
            m_keystrokeRecordButton->releaseKeyboard();
        }
        updateKeystrokeButtonText();
    }

    QWidget::closeEvent(event);
    emit windowClosed();
}

void SettingsWindow::loadWorkingCopy(const AppConfig &appConfig, const std::vector<Action> &actions, const std::vector<Menu> &menus)
{
    m_appConfig = appConfig;
    m_actions = actions;
    m_menus = menus;

    if (m_darkModeCheck) {
        m_darkModeCheck->setChecked(m_appConfig.darkMode);
        m_darkModeCheck->setText(m_appConfig.darkMode ? "Light Mode" : "Dark Mode");
    }

    m_selectionKind = SelectionKind::None;
    
    updateKeystrokeButtonText();
    
    refreshLists();

    if (!m_menus.empty())
    {
        setSelectedMenu(0);
    }
    else if (!m_actions.empty())
    {
        setSelectedAction(0);
    }
    else
    {
        refreshEditors();
    }
}

void SettingsWindow::exportWorkingCopy(AppConfig &appConfig, std::vector<Action> &actions, std::vector<Menu> &menus) const
{
    appConfig = m_appConfig;
    if (m_darkModeCheck) {
        appConfig.darkMode = m_darkModeCheck->isChecked();
    }
    actions = m_actions;
    menus = m_menus;
}

void SettingsWindow::refreshLists()
{
    refreshMenuList();
    refreshActionList();
}

void SettingsWindow::refreshMenuList()
{
    QSignalBlocker blocker(m_menuList);
    const int selected = currentMenuIndex();
    m_menuList->clear();
    for (const Menu &menu : m_menus)
    {
        m_menuList->addItem(QString::fromStdString(menu.name()));
    }
    if (selected >= 0 && selected < m_menuList->count())
    {
        m_menuList->setCurrentRow(selected);
    }
}

void SettingsWindow::refreshActionList()
{
    QSignalBlocker blocker(m_actionList);
    const int selected = currentActionIndex();
    m_actionList->clear();
    for (const Action &action : m_actions)
    {
        QString label = QString::fromStdString(action.name());
        if (action.channel() != 0)
        {
            label += QString(" · ch %1").arg(action.channel());
        }
        m_actionList->addItem(label);
    }
    if (selected >= 0 && selected < m_actionList->count())
    {
        m_actionList->setCurrentRow(selected);
    }
}

void SettingsWindow::refreshEditors()
{
    if (m_selectionKind == SelectionKind::Menu && currentMenuIndex() >= 0)
    {
        m_editorStack->setCurrentWidget(m_menuEditor);
        refreshMenuEditor();
        return;
    }

    if (m_selectionKind == SelectionKind::Action && currentActionIndex() >= 0)
    {
        m_editorStack->setCurrentWidget(m_actionEditor);
        refreshActionEditor();
        return;
    }

    m_editorStack->setCurrentWidget(m_emptyEditor);
}

void SettingsWindow::refreshMenuEditor()
{
    const int index = currentMenuIndex();
    if (index < 0)
    {
        return;
    }

    const QSignalBlocker nameBlocker(m_menuNameEdit);
    const QSignalBlocker releaseBlocker(m_executeOnReleaseCheck);
    const QSignalBlocker exitBlocker(m_exitOnActionCheck);
    const QSignalBlocker centerMouseBlocker(m_centerMouseOnOpenCheck);
    const QSignalBlocker restoreMouseBlocker(m_restoreMouseOnCloseCheck);
    m_menuNameEdit->setText(QString::fromStdString(m_menus[index].name()));
    m_executeOnReleaseCheck->setChecked(m_menus[index].executeOnRelease());
    m_exitOnActionCheck->setChecked(m_menus[index].exitOnAction());
    m_centerMouseOnOpenCheck->setChecked(m_menus[index].centerMouseOnOpen());
    m_restoreMouseOnCloseCheck->setChecked(m_menus[index].restoreMouseOnClose());

    updateKeystrokeButtonText();
    refreshSlotList();
    refreshSlotActionCombo();
}

void SettingsWindow::refreshActionEditor()
{
    const int index = currentActionIndex();
    if (index < 0)
    {
        return;
    }

    const QSignalBlocker nameBlocker(m_actionNameEdit);
    const QSignalBlocker iconBlocker(m_actionIconEdit);
    const QSignalBlocker channelBlocker(m_actionChannelSpin);
    m_actionNameEdit->setText(QString::fromStdString(m_actions[index].name()));
    m_actionIconEdit->setText(QString::fromStdString(m_actions[index].iconFilepath()));
    refreshActionIconPreview();
    const int channel = static_cast<int>(m_actions[index].channel());
    m_actionChannelSpin->setValue(channel);
    if (channel == 0)
    {
        m_actionChannelHelpLabel->setText(
            "Independent: runs without blocking other channel-0 actions.");
    }
    else
    {
        m_actionChannelHelpLabel->setText(
            QString("Shared FIFO: only one action on channel %1 runs at a time.").arg(channel));
    }
    refreshActionItemList();
    refreshActionSummary();
    refreshItemDetail();
}

void SettingsWindow::refreshSlotList()
{
    const int menuIndex = currentMenuIndex();
    if (menuIndex < 0)
    {
        return;
    }

    QSignalBlocker blocker(m_slotList);
    const int selected = m_slotList->currentRow();
    m_slotList->clear();
    const std::vector<std::string> &actionIds = m_menus[menuIndex].actionIds();
    for (const std::string &actionId : actionIds)
    {
        QString label = "Missing Action";
        for (const Action &action : m_actions)
        {
            if (action.id() == actionId)
            {
                label = QString::fromStdString(action.name());
                break;
            }
        }
        m_slotList->addItem(label);
    }
    if (selected >= 0 && selected < m_slotList->count())
    {
        m_slotList->setCurrentRow(selected);
    }
    else if (m_slotList->count() > 0)
    {
        m_slotList->setCurrentRow(0);
    }
}

void SettingsWindow::refreshActionItemList()
{
    const int actionIndex = currentActionIndex();
    if (actionIndex < 0)
    {
        return;
    }

    QSignalBlocker blocker(m_actionItemList);
    const int selected = currentActionItemIndex();
    m_actionItemList->clear();
    for (const auto &item : m_actions[actionIndex].items())
    {
        m_actionItemList->addItem(describeActionItem(item.get()));
    }
    if (selected >= 0 && selected < m_actionItemList->count())
    {
        m_actionItemList->setCurrentRow(selected);
    }
    else if (m_actionItemList->count() > 0)
    {
        m_actionItemList->setCurrentRow(0);
    }
}

void SettingsWindow::refreshActionSummary()
{
    const int actionIndex = currentActionIndex();
    if (actionIndex < 0)
    {
        m_actionSequenceLabel->clear();
        return;
    }

    QStringList parts;
    for (const auto &item : m_actions[actionIndex].items())
    {
        parts.append(describeActionItem(item.get()));
    }

    if (parts.isEmpty())
    {
        m_actionSequenceLabel->setText("No action items yet.");
        return;
    }

    m_actionSequenceLabel->setText(parts.join("  ->  "));
}

void SettingsWindow::refreshActionIconPreview()
{
    if (m_actionIconPreview == nullptr)
    {
        return;
    }

    const int index = currentActionIndex();
    if (index < 0 || m_actions[index].iconFilepath().empty())
    {
        m_actionIconPreview->setPixmap(QPixmap());
        m_actionIconPreview->setText("—");
        return;
    }

    const QPixmap pixmap(QString::fromStdString(m_actions[index].iconFilepath()));
    if (pixmap.isNull())
    {
        m_actionIconPreview->setPixmap(QPixmap());
        m_actionIconPreview->setText("?");
        return;
    }

    m_actionIconPreview->setText(QString());
    m_actionIconPreview->setPixmap(
        pixmap.scaled(m_actionIconPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void SettingsWindow::refreshSlotActionCombo()
{
    QSignalBlocker blocker(m_slotActionCombo);
    m_isRefreshing = true;
    m_slotActionCombo->clear();
    for (const Action &action : m_actions)
    {
        m_slotActionCombo->addItem(QString::fromStdString(action.name()), QString::fromStdString(action.id()));
    }

    const int menuIndex = currentMenuIndex();
    const int slotIndex = m_slotList->currentRow();
    if (menuIndex >= 0 && slotIndex >= 0)
    {
        const QString currentId = QString::fromStdString(m_menus[menuIndex].actionId(slotIndex));
        const int comboIndex = m_slotActionCombo->findData(currentId);
        if (comboIndex >= 0)
        {
            m_slotActionCombo->setCurrentIndex(comboIndex);
        }
    }
    m_isRefreshing = false;
}

void SettingsWindow::refreshMenuTargetCombo()
{
    QSignalBlocker blocker(m_menuTargetCombo);
    m_isRefreshing = true;
    m_menuTargetCombo->clear();
    for (const Menu &menu : m_menus)
    {
        m_menuTargetCombo->addItem(QString::fromStdString(menu.name()), QString::fromStdString(menu.id()));
    }

    const int actionIndex = currentActionIndex();
    const int itemIndex = currentActionItemIndex();
    auto *item = actionIndex >= 0 ? dynamic_cast<AI_Menu *>(m_actions[actionIndex].item(itemIndex)) : nullptr;
    if (item != nullptr)
    {
        const int comboIndex = m_menuTargetCombo->findData(QString::fromStdString(item->menuId));
        if (comboIndex >= 0)
        {
            m_menuTargetCombo->setCurrentIndex(comboIndex);
        }
    }
    m_isRefreshing = false;
}

void SettingsWindow::refreshItemDetail()
{
    const int actionIndex = currentActionIndex();
    const int itemIndex = currentActionItemIndex();
    if (actionIndex < 0 || itemIndex < 0)
    {
        m_itemDetailStack->setCurrentWidget(m_itemNonePage);
        return;
    }

    ActionItem *item = m_actions[actionIndex].item(itemIndex);
    if (auto *launch = dynamic_cast<AI_LaunchApp *>(item))
    {
        const QSignalBlocker presetBlocker(m_launchPresetCombo);
        const QSignalBlocker pathBlocker(m_launchCustomPathEdit);
        const QString presetId = QString::fromStdString(launch->presetId);
        int comboIndex = m_launchPresetCombo->findData(presetId);
        if (comboIndex < 0)
        {
            comboIndex = m_launchPresetCombo->findData("custom");
        }
        m_launchPresetCombo->setCurrentIndex(comboIndex);
        const bool isCustom = launch->presetId == "custom";
        m_launchCustomPathEdit->setText(QString::fromStdString(launch->customTarget));
        m_launchCustomPathEdit->setEnabled(isCustom);
        m_browseLaunchAppButton->setEnabled(isCustom);
        m_itemDetailStack->setCurrentWidget(m_itemLaunchAppPage);
        return;
    }

    if (auto *script = dynamic_cast<AI_Script *>(item))
    {
        QSignalBlocker blocker(m_scriptPathEdit);
        m_scriptPathEdit->setText(QString::fromStdString(script->filepath));
        m_itemDetailStack->setCurrentWidget(m_itemScriptPage);
        return;
    }

    if (auto *delay = dynamic_cast<AI_Delay *>(item))
    {
        QSignalBlocker blocker(m_delaySpin);
        m_delaySpin->setValue(delay->durationMs);
        m_itemDetailStack->setCurrentWidget(m_itemDelayPage);
        return;
    }

    if (dynamic_cast<AI_Close *>(item) != nullptr)
    {
        m_itemDetailStack->setCurrentWidget(m_itemClosePage);
        return;
    }

    if (dynamic_cast<AI_Menu *>(item) != nullptr)
    {
        refreshMenuTargetCombo();
        m_itemDetailStack->setCurrentWidget(m_itemMenuPage);
        return;
    }

    if (auto *keystroke = dynamic_cast<AI_Keystroke *>(item))
    {
        const QSignalBlocker ctrlBlocker(m_keystrokeCtrlCheck);
        const QSignalBlocker altBlocker(m_keystrokeAltCheck);
        const QSignalBlocker shiftBlocker(m_keystrokeShiftCheck);
        const QSignalBlocker winBlocker(m_keystrokeWinCheck);
        const QSignalBlocker keyBlocker(m_keystrokeKeyCombo);
        const QSignalBlocker holdBlocker(m_keystrokeHoldSpin);
        const QSignalBlocker proceedBlocker(m_keystrokeProceedCheck);
        m_keystrokeCtrlCheck->setChecked((keystroke->modifiers & 0x0002) != 0);
        m_keystrokeAltCheck->setChecked((keystroke->modifiers & 0x0001) != 0);
        m_keystrokeShiftCheck->setChecked((keystroke->modifiers & 0x0004) != 0);
        m_keystrokeWinCheck->setChecked((keystroke->modifiers & 0x0008) != 0);
        const int comboIndex = m_keystrokeKeyCombo->findData(keystroke->keycode);
        m_keystrokeKeyCombo->setCurrentIndex(comboIndex >= 0 ? comboIndex : 0);
        m_keystrokeHoldSpin->setValue(keystroke->holdDurationSec);
        m_keystrokeProceedCheck->setChecked(keystroke->proceed);
        m_itemDetailStack->setCurrentWidget(m_itemKeystrokePage);
        return;
    }

    if (auto *mouseMove = dynamic_cast<AI_MouseMove *>(item))
    {
        const QSignalBlocker xBlocker(m_mouseMoveXSpin);
        const QSignalBlocker yBlocker(m_mouseMoveYSpin);
        m_mouseMoveXSpin->setValue(mouseMove->x);
        m_mouseMoveYSpin->setValue(mouseMove->y);
        m_itemDetailStack->setCurrentWidget(m_itemMouseMovePage);
        return;
    }

    if (auto *mouseButton = dynamic_cast<AI_MouseButton *>(item))
    {
        const QSignalBlocker buttonBlocker(m_mouseButtonCombo);
        const QSignalBlocker ctrlBlocker(m_mouseButtonCtrlCheck);
        const QSignalBlocker altBlocker(m_mouseButtonAltCheck);
        const QSignalBlocker shiftBlocker(m_mouseButtonShiftCheck);
        const QSignalBlocker winBlocker(m_mouseButtonWinCheck);
        const QSignalBlocker holdBlocker(m_mouseButtonHoldSpin);
        const QSignalBlocker proceedBlocker(m_mouseButtonProceedCheck);
        const int buttonIndex = m_mouseButtonCombo->findData(mouseButton->button);
        m_mouseButtonCombo->setCurrentIndex(buttonIndex >= 0 ? buttonIndex : 0);
        m_mouseButtonCtrlCheck->setChecked((mouseButton->modifiers & 0x0002) != 0);
        m_mouseButtonAltCheck->setChecked((mouseButton->modifiers & 0x0001) != 0);
        m_mouseButtonShiftCheck->setChecked((mouseButton->modifiers & 0x0004) != 0);
        m_mouseButtonWinCheck->setChecked((mouseButton->modifiers & 0x0008) != 0);
        m_mouseButtonHoldSpin->setValue(mouseButton->holdDurationSec);
        m_mouseButtonProceedCheck->setChecked(mouseButton->proceed);
        m_itemDetailStack->setCurrentWidget(m_itemMouseButtonPage);
        return;
    }

    if (auto *cancel = dynamic_cast<AI_Cancel *>(item))
    {
        const QSignalBlocker levelBlocker(m_cancelLevelCombo);
        const QSignalBlocker channelBlocker(m_cancelChannelSpin);
        const int levelIndex = m_cancelLevelCombo->findData(static_cast<int>(cancel->level));
        m_cancelLevelCombo->setCurrentIndex(levelIndex >= 0 ? levelIndex : 0);
        m_cancelChannelSpin->setValue(static_cast<int>(cancel->channel));
        updateCancelChannelVisibility(cancel->level);
        if (cancel->level == CancelLevel::MostRecent)
        {
            m_cancelHelpLabel->setText(
                "Cancels the most recently submitted action on the chosen channel. "
                "Use channel 0 for the most recent action on any channel. "
                "Never cancels the action that contains this item.");
        }
        else if (cancel->level == CancelLevel::Channel)
        {
            m_cancelHelpLabel->setText("Cancels every cancelable action on the chosen channel.");
        }
        else
        {
            m_cancelHelpLabel->setText("Cancels every cancelable action.");
        }
        m_itemDetailStack->setCurrentWidget(m_itemCancelPage);
        return;
    }

    if (auto *recent = dynamic_cast<AI_NthRecent *>(item))
    {
        const QSignalBlocker blocker(m_nthSpin);
        m_nthSpin->setValue(std::max(1, recent->n));
        m_nthHelpLabel->setText("Runs the Nth most recently launched action from the wheel (1 = most recent).");
        m_resetFrequenciesButton->hide();
        m_itemDetailStack->setCurrentWidget(m_itemNthPage);
        return;
    }

    if (auto *frequent = dynamic_cast<AI_NthFrequent *>(item))
    {
        const QSignalBlocker blocker(m_nthSpin);
        m_nthSpin->setValue(std::max(1, frequent->n));
        m_nthHelpLabel->setText("Runs the Nth most frequently launched action from the wheel (1 = most used).");
        m_resetFrequenciesButton->show();
        m_itemDetailStack->setCurrentWidget(m_itemNthPage);
        return;
    }

    if (auto *socket = dynamic_cast<AI_Socket *>(item))
    {
        const QSignalBlocker protocolBlocker(m_socketProtocolCombo);
        const QSignalBlocker destinationBlocker(m_socketDestinationEdit);
        const QSignalBlocker messageBlocker(m_socketMessageEdit);
        const QSignalBlocker methodBlocker(m_socketHttpMethodEdit);
        const int protocolIndex =
            m_socketProtocolCombo->findData(static_cast<int>(socket->protocol));
        m_socketProtocolCombo->setCurrentIndex(protocolIndex >= 0 ? protocolIndex : 0);
        m_socketDestinationEdit->setText(QString::fromStdString(socket->outputDst));
        m_socketMessageEdit->setText(QString::fromStdString(socket->socketMsg));
        m_socketHttpMethodEdit->setText(
            socket->httpMethod.empty() ? QStringLiteral("POST")
                                       : QString::fromStdString(socket->httpMethod));
        const bool isHttp = socket->protocol == Platform::SocketProtocol::Http;
        m_socketHttpMethodEdit->setEnabled(isHttp);
        m_itemDetailStack->setCurrentWidget(m_itemSocketPage);
        return;
    }

    if (auto *search = dynamic_cast<AI_Search *>(item))
    {
        const QSignalBlocker actionsBlocker(m_searchActionsCheck);
        const QSignalBlocker programsBlocker(m_searchProgramsCheck);
        const QSignalBlocker menusBlocker(m_searchMenusCheck);
        const QSignalBlocker webBlocker(m_searchWebCheck);
        const QSignalBlocker urlBlocker(m_searchWebUrlEdit);
        m_searchActionsCheck->setChecked(search->config.searchActions);
        m_searchProgramsCheck->setChecked(search->config.searchPrograms);
        m_searchMenusCheck->setChecked(search->config.searchMenus);
        m_searchWebCheck->setChecked(search->config.webSearch);
        m_searchWebUrlEdit->setText(QString::fromStdString(search->config.webSearchUrl));
        m_searchWebUrlEdit->setEnabled(search->config.webSearch);
        m_itemDetailStack->setCurrentWidget(m_itemSearchPage);
        return;
    }

    if (auto *release = dynamic_cast<AI_KeyRelease *>(item))
    {
        m_keyReleaseHelpLabel->setText(
            QString("Internal key release (keycode %1, mods %2).\n"
                    "Created automatically by Press Keystroke holds for cancel-flush / "
                    "delayed release. Not added from the item picker.")
                .arg(release->keycode)
                .arg(release->modifiers));
        m_itemDetailStack->setCurrentWidget(m_itemKeyReleasePage);
        return;
    }

    if (auto *mouseRelease = dynamic_cast<AI_MouseButtonRelease *>(item))
    {
        const char *name = "Left";
        if (mouseRelease->button == 1)
        {
            name = "Right";
        }
        else if (mouseRelease->button == 2)
        {
            name = "Middle";
        }
        m_keyReleaseHelpLabel->setText(
            QString("Internal mouse button release (%1, mods %2).\n"
                    "Created automatically by Mouse Button holds for cancel-flush / "
                    "delayed release. Not added from the item picker.")
                .arg(name)
                .arg(mouseRelease->modifiers));
        m_itemDetailStack->setCurrentWidget(m_itemKeyReleasePage);
        return;
    }

    m_itemDetailStack->setCurrentWidget(m_itemNonePage);
}

QString SettingsWindow::describeActionItem(const ActionItem *item) const
{
    if (item == nullptr)
    {
        return "Unknown";
    }

    switch (item->kind())
    {
    case ActionItemKind::LaunchApp:
    {
        const auto *launch = static_cast<const AI_LaunchApp *>(item);
        if (launch->presetId == "custom")
        {
            const QString path = QString::fromStdString(launch->customTarget).trimmed();
            if (path.isEmpty())
            {
                return "Launch App: choose app";
            }
            const QString fileName = QFileInfo(path).fileName();
            return fileName.isEmpty() ? QString("Launch App: %1").arg(path) : QString("Launch App: %1").arg(fileName);
        }
        return QString("Launch App: %1").arg(launchPresetDisplayName(launch->presetId));
    }
    case ActionItemKind::Script:
    {
        const auto *script = static_cast<const AI_Script *>(item);
        const QString path = QString::fromStdString(script->filepath).trimmed();
        if (path.isEmpty())
        {
            return "Custom Script/App: choose file";
        }
        const QString fileName = QFileInfo(path).fileName();
        return fileName.isEmpty() ? QString("Custom Script/App: %1").arg(path) : QString("Custom Script/App: %1").arg(fileName);
    }
    case ActionItemKind::Delay:
        return QString("Delay: %1 ms").arg(static_cast<const AI_Delay *>(item)->durationMs);
    case ActionItemKind::Keystroke:
    {
        const auto *keystroke = static_cast<const AI_Keystroke *>(item);
        QStringList keys;
        if ((keystroke->modifiers & 0x0002) != 0)
            keys << "Ctrl";
        if ((keystroke->modifiers & 0x0001) != 0)
            keys << "Alt";
        if ((keystroke->modifiers & 0x0004) != 0)
            keys << "Shift";
        if ((keystroke->modifiers & 0x0008) != 0)
            keys << "Win";
        if (keystroke->keycode != 0)
            keys << keyDisplayName(keystroke->keycode);
        return keys.isEmpty() ? "Press Keystroke: choose keys" : QString("Press Keystroke: %1").arg(keys.join("+"));
    }
    case ActionItemKind::Menu:
    {
        const auto *menuItem = static_cast<const AI_Menu *>(item);
        for (const Menu &menu : m_menus)
        {
            if (menu.id() == menuItem->menuId)
            {
                return QString("Open Menu: %1").arg(QString::fromStdString(menu.name()));
            }
        }
        return "Open Menu: missing";
    }
    case ActionItemKind::Close:
        return "Close launcher";
    case ActionItemKind::MouseMove:
    {
        const auto *move = static_cast<const AI_MouseMove *>(item);
        return QString("Mouse Move: (%1, %2)").arg(move->x).arg(move->y);
    }
    case ActionItemKind::MouseButton:
    {
        const auto *button = static_cast<const AI_MouseButton *>(item);
        const char *name = "Left";
        if (button->button == 1)
        {
            name = "Right";
        }
        else if (button->button == 2)
        {
            name = "Middle";
        }
        QStringList keys;
        if ((button->modifiers & 0x0002) != 0)
            keys << "Ctrl";
        if ((button->modifiers & 0x0001) != 0)
            keys << "Alt";
        if ((button->modifiers & 0x0004) != 0)
            keys << "Shift";
        if ((button->modifiers & 0x0008) != 0)
            keys << "Win";
        keys << name;
        return QString("Mouse Button: %1").arg(keys.join("+"));
    }
    case ActionItemKind::MouseButtonRelease:
    {
        const auto *release = static_cast<const AI_MouseButtonRelease *>(item);
        const char *name = "Left";
        if (release->button == 1)
        {
            name = "Right";
        }
        else if (release->button == 2)
        {
            name = "Middle";
        }
        return QString("Mouse Button Release: %1").arg(name);
    }
    case ActionItemKind::Cancel:
    {
        const auto *cancel = static_cast<const AI_Cancel *>(item);
        switch (cancel->level)
        {
        case CancelLevel::Channel:
            return QString("Cancel Channel: %1").arg(cancel->channel);
        case CancelLevel::All:
            return "Cancel All";
        case CancelLevel::MostRecent:
        default:
            if (cancel->channel == 0)
            {
                return "Cancel Most Recent (any channel)";
            }
            return QString("Cancel Most Recent: channel %1").arg(cancel->channel);
        }
    }
    case ActionItemKind::NthRecent:
        return QString("Nth Recent: %1").arg(static_cast<const AI_NthRecent *>(item)->n);
    case ActionItemKind::NthFrequent:
        return QString("Nth Frequent: %1").arg(static_cast<const AI_NthFrequent *>(item)->n);
    case ActionItemKind::Socket:
    {
        const auto *socket = static_cast<const AI_Socket *>(item);
        const char *proto = "UDP";
        switch (socket->protocol)
        {
        case Platform::SocketProtocol::Tcp:
            proto = "TCP";
            break;
        case Platform::SocketProtocol::Http:
            proto = "HTTP";
            break;
        case Platform::SocketProtocol::WebSocket:
            proto = "WebSocket";
            break;
        case Platform::SocketProtocol::Udp:
        default:
            proto = "UDP";
            break;
        }
        const QString dst = QString::fromStdString(socket->outputDst).trimmed();
        if (dst.isEmpty())
        {
            return QString("Socket Send (%1): set destination").arg(proto);
        }
        return QString("Socket Send (%1): %2").arg(proto, dst);
    }
    case ActionItemKind::KeyRelease:
    {
        const auto *release = static_cast<const AI_KeyRelease *>(item);
        return QString("Key Release: %1").arg(keyDisplayName(release->keycode));
    }
    case ActionItemKind::Search:
    {
        const auto *search = static_cast<const AI_Search *>(item);
        QStringList filters;
        if (search->config.searchActions)
            filters << "Actions";
        if (search->config.searchPrograms)
            filters << "Programs";
        if (search->config.searchMenus)
            filters << "Menus";
        if (search->config.webSearch)
            filters << "Web";
        return filters.isEmpty() ? "Search Palette: no filters enabled"
                                 : QString("Search Palette: %1").arg(filters.join(", "));
    }
    default:
        return "Unsupported";
    }
}

QString SettingsWindow::launchPresetDisplayName(const std::string &presetId) const
{
    for (const LaunchPresetOption &option : kLaunchPresetOptions)
    {
        if (presetId == option.id)
        {
            return option.label;
        }
    }

    return "Custom App...";
}

QString SettingsWindow::launchPresetTarget(const std::string &presetId) const
{
    for (const LaunchPresetOption &option : kLaunchPresetOptions)
    {
        if (presetId == option.id)
        {
            return option.target;
        }
    }

    return {};
}

QString SettingsWindow::keyDisplayName(int vk) const
{
    if (vk >= 'A' && vk <= 'Z')
    {
        return QString(QChar(vk));
    }
    if (vk >= '0' && vk <= '9')
    {
        return QString(QChar(vk));
    }
    // VK_F1 (0x70) through VK_F24 (0x87)
    if (vk >= 0x70 && vk <= 0x87)
    {
        return QString("F%1").arg(vk - 0x6F);
    }
    if (vk >= 0x60 && vk <= 0x69)
    {
        return QString("Numpad %1").arg(vk - 0x60);
    }

    switch (vk)
    {
    case 0x0D:
        return "Enter";
    case 0x09:
        return "Tab";
    case 0x1B:
        return "Esc";
    case 0x20:
        return "Space";
    case 0x25:
        return "Left";
    case 0x26:
        return "Up";
    case 0x27:
        return "Right";
    case 0x28:
        return "Down";
    case 0x08:
        return "Backspace";
    case 0x2E:
        return "Delete";
    case 0x2D:
        return "Insert";
    case 0x24:
        return "Home";
    case 0x23:
        return "End";
    case 0x21:
        return "Page Up";
    case 0x22:
        return "Page Down";
    case 0x14:
        return "Caps Lock";
    case 0x90:
        return "Num Lock";
    case 0x91:
        return "Scroll Lock";
    case 0x2C:
        return "Print Screen";
    case 0x13:
        return "Pause";
    case 0x5D:
        return "Menu";
    // OEM / punctuation (US layout labels; Shift variants noted where useful)
    case 0xBA:
        return "; / :";
    case 0xBB:
        return "= / +";
    case 0xBC:
        return ", / <";
    case 0xBD:
        return "- / _";
    case 0xBE:
        return ". / >";
    case 0xBF:
        return "Slash / ?";
    case 0xC0:
        return "` / ~";
    case 0xDB:
        return "[ / {";
    case 0xDC:
        return "\\ / |";
    case 0xDD:
        return "] / }";
    case 0xDE:
        return "' / \"";
    // Numpad operators
    case 0x6A:
        return "Numpad *";
    case 0x6B:
        return "Numpad +";
    case 0x6D:
        return "Numpad -";
    case 0x6E:
        return "Numpad .";
    case 0x6F:
        return "Numpad /";
    // Media
    case 0xAD:
        return "Mute";
    case 0xAE:
        return "Volume Down";
    case 0xAF:
        return "Volume Up";
    case 0xB0:
        return "Next Track";
    case 0xB1:
        return "Previous Track";
    case 0xB2:
        return "Stop Media";
    case 0xB3:
        return "Play/Pause";
    case 0xB5:
        return "Media Select";
    // Browser / launch
    case 0xA6:
        return "Browser Back";
    case 0xA7:
        return "Browser Forward";
    case 0xA8:
        return "Browser Refresh";
    case 0xA9:
        return "Browser Stop";
    case 0xAA:
        return "Browser Search";
    case 0xAB:
        return "Browser Favorites";
    case 0xAC:
        return "Browser Home";
    case 0xB4:
        return "Launch Mail";
    case 0xB6:
        return "Launch App 1";
    case 0xB7:
        return "Launch App 2";
    default:
        return QString("Key %1").arg(vk);
    }
}

void SettingsWindow::populateLaunchPresetCombo()
{
    if (m_launchPresetCombo == nullptr)
    {
        return;
    }

    m_launchPresetCombo->clear();
    for (const LaunchPresetOption &option : kLaunchPresetOptions)
    {
        m_launchPresetCombo->addItem(option.label, option.id);
    }
}

void SettingsWindow::populateKeystrokeKeyCombo()
{
    if (m_keystrokeKeyCombo == nullptr)
    {
        return;
    }

    m_keystrokeKeyCombo->clear();
    for (int key = 'A'; key <= 'Z'; ++key)
    {
        m_keystrokeKeyCombo->addItem(QString(QChar(key)), key);
    }
    for (int key = '0'; key <= '9'; ++key)
    {
        m_keystrokeKeyCombo->addItem(QString(QChar(key)), key);
    }

    // Editing / navigation
    const std::vector<int> navKeys = {
        0x0D, // Enter
        0x09, // Tab
        0x1B, // Esc
        0x20, // Space
        0x08, // Backspace
        0x2E, // Delete
        0x2D, // Insert
        0x25, // Left
        0x26, // Up
        0x27, // Right
        0x28, // Down
        0x24, // Home
        0x23, // End
        0x21, // Page Up
        0x22, // Page Down
        0x14, // Caps Lock
        0x90, // Num Lock
        0x91, // Scroll Lock
        0x2C, // Print Screen
        0x13, // Pause
        0x5D, // Menu (Apps)
    };
    for (int key : navKeys)
    {
        m_keystrokeKeyCombo->addItem(keyDisplayName(key), key);
    }

    // F1–F24
    for (int key = 0x70; key <= 0x87; ++key)
    {
        m_keystrokeKeyCombo->addItem(keyDisplayName(key), key);
    }

    // OEM punctuation (US layout). Use Shift modifier for the shifted glyph
    // (e.g. Shift + "- / _" for underscore, Shift + "= / +" for plus).
    const std::vector<int> symbolKeys = {
        0xBD, // - / _
        0xBB, // = / +
        0xBA, // ; / :
        0xDE, // ' / "
        0xBC, // , / <
        0xBE, // . / >
        0xBF, // / / ?
        0xC0, // ` / ~
        0xDB, // [ / {
        0xDD, // ] / }
        0xDC, // \ / |
    };
    for (int key : symbolKeys)
    {
        m_keystrokeKeyCombo->addItem(keyDisplayName(key), key);
    }

    // Numpad 0–9 and operators
    for (int key = 0x60; key <= 0x69; ++key)
    {
        m_keystrokeKeyCombo->addItem(keyDisplayName(key), key);
    }
    const std::vector<int> numpadOps = {
        0x6A, // *
        0x6B, // +
        0x6D, // -
        0x6E, // .
        0x6F, // /
    };
    for (int key : numpadOps)
    {
        m_keystrokeKeyCombo->addItem(keyDisplayName(key), key);
    }

    // Windows media keys. Sent without modifiers.
    const std::vector<int> mediaKeys = {
        0xB3, // Play/Pause
        0xB0, // Next Track
        0xB1, // Previous Track
        0xB2, // Stop
        0xAF, // Volume Up
        0xAE, // Volume Down
        0xAD, // Mute
        0xB5, // Media Select
    };
    for (int key : mediaKeys)
    {
        m_keystrokeKeyCombo->addItem(QString("Media: %1").arg(keyDisplayName(key)), key);
    }

    // Browser / launch keys (same SendInput path as media).
    const std::vector<int> browserKeys = {
        0xA6, // Browser Back
        0xA7, // Browser Forward
        0xA8, // Browser Refresh
        0xA9, // Browser Stop
        0xAA, // Browser Search
        0xAB, // Browser Favorites
        0xAC, // Browser Home
        0xB4, // Launch Mail
        0xB6, // Launch App 1
        0xB7, // Launch App 2
    };
    for (int key : browserKeys)
    {
        m_keystrokeKeyCombo->addItem(keyDisplayName(key), key);
    }
}

void SettingsWindow::setSelectedMenu(int index)
{
    if (index < 0 || index >= static_cast<int>(m_menus.size()))
    {
        return;
    }

    m_selectionKind = SelectionKind::Menu;
    {
        QSignalBlocker blocker(m_actionList);
        m_actionList->clearSelection();
        m_actionList->setCurrentRow(-1);
    }
    m_menuList->setCurrentRow(index);
    refreshEditors();
}

void SettingsWindow::setSelectedAction(int index)
{
    if (index < 0 || index >= static_cast<int>(m_actions.size()))
    {
        return;
    }

    m_selectionKind = SelectionKind::Action;
    {
        QSignalBlocker blocker(m_menuList);
        m_menuList->clearSelection();
        m_menuList->setCurrentRow(-1);
    }
    m_actionList->setCurrentRow(index);
    refreshEditors();
}

int SettingsWindow::currentMenuIndex() const
{
    return m_selectionKind == SelectionKind::Menu ? m_menuList->currentRow() : -1;
}

int SettingsWindow::currentActionIndex() const
{
    return m_selectionKind == SelectionKind::Action ? m_actionList->currentRow() : -1;
}

int SettingsWindow::currentActionItemIndex() const
{
    return m_actionItemList->currentRow();
}

void SettingsWindow::deleteActionReferences(const std::string &actionId)
{
    for (Menu &menu : m_menus)
    {
        std::vector<std::string> keptIds;
        for (const std::string &slotActionId : menu.actionIds())
        {
            if (slotActionId != actionId)
            {
                keptIds.push_back(slotActionId);
            }
        }

        Menu replacement(menu.triggerMod(),
                         menu.triggerVk(),
                         menu.executeOnRelease(),
                         menu.exitOnAction(),
                         menu.centerMouseOnOpen(),
                         menu.restoreMouseOnClose(),
                         menu.name(),
                         keptIds,
                         menu.id());
        menu = replacement;
    }
}

void SettingsWindow::clearMenuReferences(const std::string &menuId)
{
    for (Action &action : m_actions)
    {
        for (const auto &itemPtr : action.items())
        {
            auto *menuItem = dynamic_cast<AI_Menu *>(itemPtr.get());
            if (menuItem != nullptr && menuItem->menuId == menuId)
            {
                menuItem->menuId.clear();
            }
        }
    }
}

std::string SettingsWindow::makeUniqueActionId(const QString &seed) const
{
    QString base = slugify(seed, "action");
    QString candidate = base;
    int suffix = 2;

    while (true)
    {
        bool exists = false;
        for (const Action &action : m_actions)
        {
            if (action.id() == candidate.toStdString())
            {
                exists = true;
                break;
            }
        }
        if (!exists)
        {
            return candidate.toStdString();
        }
        candidate = QString("%1-%2").arg(base).arg(suffix++);
    }
}

std::string SettingsWindow::makeUniqueMenuId(const QString &seed) const
{
    QString base = slugify(seed, "menu");
    QString candidate = base;
    int suffix = 2;

    while (true)
    {
        bool exists = false;
        for (const Menu &menu : m_menus)
        {
            if (menu.id() == candidate.toStdString())
            {
                exists = true;
                break;
            }
        }
        if (!exists)
        {
            return candidate.toStdString();
        }
        candidate = QString("%1-%2").arg(base).arg(suffix++);
    }
}

bool SettingsWindow::validateWorkingCopy(QString &errorMessage) const
{
    std::vector<std::string> actionIds;
    for (const Action &action : m_actions)
    {
        if (action.id().empty())
        {
            errorMessage = "Every action must have a non-empty ID.";
            return false;
        }
        if (std::find(actionIds.begin(), actionIds.end(), action.id()) != actionIds.end())
        {
            errorMessage = "Action IDs must be unique.";
            return false;
        }
        actionIds.push_back(action.id());

        for (const auto &itemPtr : action.items())
        {
            if (itemPtr == nullptr)
            {
                continue;
            }

            if (itemPtr->kind() == ActionItemKind::Script)
            {
                const auto *script = static_cast<const AI_Script *>(itemPtr.get());
                if (QString::fromStdString(script->filepath).trimmed().isEmpty())
                {
                    errorMessage = QString("Advanced script/app item in '%1' has an empty path.").arg(QString::fromStdString(action.name()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::LaunchApp)
            {
                const auto *launch = static_cast<const AI_LaunchApp *>(itemPtr.get());
                if (launch->presetId.empty())
                {
                    errorMessage = QString("Launch App item in '%1' is missing its preset.").arg(QString::fromStdString(action.name()));
                    return false;
                }
                if (launch->presetId == "custom" && QString::fromStdString(launch->customTarget).trimmed().isEmpty())
                {
                    errorMessage = QString("Launch App item in '%1' needs a custom app path.").arg(QString::fromStdString(action.name()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::Delay)
            {
                const auto *delay = static_cast<const AI_Delay *>(itemPtr.get());
                if (delay->durationMs < 0)
                {
                    errorMessage = QString("Delay item in '%1' must be non-negative.").arg(QString::fromStdString(action.name()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::Keystroke)
            {
                const auto *keystroke = static_cast<const AI_Keystroke *>(itemPtr.get());
                if (keystroke->keycode == 0)
                {
                    errorMessage = QString("Press Keystroke item in '%1' needs a main key.").arg(QString::fromStdString(action.name()));
                    return false;
                }
                if (keystroke->holdDurationSec < 0.0f)
                {
                    errorMessage = QString("Press Keystroke item in '%1' has an invalid hold time.").arg(QString::fromStdString(action.name()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::Menu)
            {
                const auto *menuItem = static_cast<const AI_Menu *>(itemPtr.get());
                bool foundMenu = false;
                for (const Menu &menu : m_menus)
                {
                    if (menu.id() == menuItem->menuId)
                    {
                        foundMenu = true;
                        break;
                    }
                }
                if (!foundMenu)
                {
                    errorMessage = QString("Menu action '%1' targets a missing menu.").arg(QString::fromStdString(action.name()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::MouseButton)
            {
                const auto *button = static_cast<const AI_MouseButton *>(itemPtr.get());
                if (button->button < 0 || button->button > 2)
                {
                    errorMessage = QString("Mouse Button item in '%1' has an invalid button.").arg(QString::fromStdString(action.name()));
                    return false;
                }
                if (button->holdDurationSec < 0.0f)
                {
                    errorMessage = QString("Mouse Button item in '%1' has an invalid hold time.").arg(QString::fromStdString(action.name()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::MouseButtonRelease)
            {
                const auto *release = static_cast<const AI_MouseButtonRelease *>(itemPtr.get());
                if (release->button < 0 || release->button > 2)
                {
                    errorMessage = QString("Mouse Button Release item in '%1' has an invalid button.").arg(QString::fromStdString(action.name()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::NthRecent)
            {
                if (static_cast<const AI_NthRecent *>(itemPtr.get())->n < 1)
                {
                    errorMessage = QString("Nth Recent item in '%1' must be >= 1.").arg(QString::fromStdString(action.name()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::NthFrequent)
            {
                if (static_cast<const AI_NthFrequent *>(itemPtr.get())->n < 1)
                {
                    errorMessage = QString("Nth Frequent item in '%1' must be >= 1.").arg(QString::fromStdString(action.name()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::Socket)
            {
                const auto *socket = static_cast<const AI_Socket *>(itemPtr.get());
                const QString destination = QString::fromStdString(socket->outputDst).trimmed();
                if (destination.isEmpty())
                {
                    errorMessage = QString("Socket Send item in '%1' needs a destination.").arg(QString::fromStdString(action.name()));
                    return false;
                }
                const bool needsUrl = socket->protocol == Platform::SocketProtocol::Http
                    || socket->protocol == Platform::SocketProtocol::WebSocket;
                if (needsUrl)
                {
                    const QString lower = destination.toLower();
                    const bool okHttp = socket->protocol == Platform::SocketProtocol::Http
                        && (lower.startsWith("http://") || lower.startsWith("https://"));
                    const bool okWs = socket->protocol == Platform::SocketProtocol::WebSocket
                        && (lower.startsWith("ws://") || lower.startsWith("wss://"));
                    if (!okHttp && !okWs)
                    {
                        errorMessage = QString(
                            "Socket Send item in '%1' needs a full URL "
                            "(http(s):// for HTTP, ws(s):// for WebSocket).")
                                           .arg(QString::fromStdString(action.name()));
                        return false;
                    }
                }
                else if (!destination.contains(':'))
                {
                    errorMessage = QString(
                        "Socket Send item in '%1' needs host:port for UDP/TCP.")
                                       .arg(QString::fromStdString(action.name()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::Search)
            {
                const auto *search = static_cast<const AI_Search *>(itemPtr.get());
                if (!search->config.searchActions && !search->config.searchPrograms
                    && !search->config.searchMenus && !search->config.webSearch)
                {
                    errorMessage = QString("Search Palette item in '%1' needs at least one filter enabled.").arg(QString::fromStdString(action.name()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::KeyRelease)
            {
                if (static_cast<const AI_KeyRelease *>(itemPtr.get())->keycode == 0)
                {
                    errorMessage = QString("Key Release item in '%1' needs a main key.").arg(QString::fromStdString(action.name()));
                    return false;
                }
            }
        }
    }

    std::vector<std::string> menuIds;
    for (int menuIndex = 0; menuIndex < static_cast<int>(m_menus.size()); ++menuIndex)
    {
        const Menu &menu = m_menus[menuIndex];
        if (menu.id().empty())
        {
            errorMessage = "Every menu must have a non-empty ID.";
            return false;
        }
        if (std::find(menuIds.begin(), menuIds.end(), menu.id()) != menuIds.end())
        {
            errorMessage = "Menu IDs must be unique.";
            return false;
        }
        menuIds.push_back(menu.id());

        if (menu.triggerVk() != 0)
        {
            for (int otherIndex = 0; otherIndex < menuIndex; ++otherIndex)
            {
                const Menu &other = m_menus[otherIndex];
                if (other.triggerVk() == menu.triggerVk() && other.triggerMod() == menu.triggerMod())
                {
                    errorMessage = QString("Menus '%1' and '%2' share the same trigger keystroke.")
                                       .arg(QString::fromStdString(other.name()))
                                       .arg(QString::fromStdString(menu.name()));
                    return false;
                }
            }
        }

        for (const std::string &actionId : menu.actionIds())
        {
            if (std::find(actionIds.begin(), actionIds.end(), actionId) == actionIds.end())
            {
                errorMessage = QString("Menu '%1' references a missing action.").arg(QString::fromStdString(menu.name()));
                return false;
            }
        }
    }

    return true;
}
