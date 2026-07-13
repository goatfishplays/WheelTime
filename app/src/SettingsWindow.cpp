#include "App/SettingsWindow.hpp"

#include <algorithm>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSplitter>
#include <QVBoxLayout>
#include <QKeyEvent>

#include "App/ActionItems.hpp"

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
}

SettingsWindow::SettingsWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("WheelTime Settings");
    resize(1000, 620);
    setObjectName("settingsWindow");

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
    auto *globalLayout = new QHBoxLayout(m_globalGroup);
    globalLayout->addWidget(new QLabel("Launcher Hotkey:"));
    m_hotkeyRecordButton = new QPushButton("Alt + Space", m_globalGroup);
    m_hotkeyRecordButton->installEventFilter(this);
    globalLayout->addWidget(m_hotkeyRecordButton);
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

    auto *menuSettingsGroup = new QGroupBox("Menu Settings", m_menuEditor);
    auto *menuForm = new QFormLayout(menuSettingsGroup);
    m_menuNameEdit = new QLineEdit(menuSettingsGroup);
    m_executeOnReleaseCheck = new QCheckBox("Execute on release", menuSettingsGroup);
    m_exitOnActionCheck = new QCheckBox("Exit on action", menuSettingsGroup);
    menuForm->addRow("Menu Name", m_menuNameEdit);
    menuForm->addRow("", m_executeOnReleaseCheck);
    menuForm->addRow("", m_exitOnActionCheck);
    menuEditorLayout->addWidget(menuSettingsGroup);

    auto *slotsGroup = new QGroupBox("Wheel Slots", m_menuEditor);
    auto *slotsLayout = new QVBoxLayout(slotsGroup);
    m_slotList = new QListWidget(slotsGroup);
    m_slotList->setObjectName("slotList");
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
    menuEditorLayout->addWidget(slotsGroup, 1);

    m_actionEditor = new QWidget(m_editorStack);
    m_actionEditor->setObjectName("actionEditor");
    auto *actionEditorLayout = new QVBoxLayout(m_actionEditor);
    actionEditorLayout->setContentsMargins(12, 12, 12, 12);
    actionEditorLayout->setSpacing(12);

    auto *actionSettingsGroup = new QGroupBox("Action Settings", m_actionEditor);
    auto *actionForm = new QFormLayout(actionSettingsGroup);
    m_actionNameEdit = new QLineEdit(actionSettingsGroup);
    actionForm->addRow("Action Name", m_actionNameEdit);
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
    actionEditorLayout->addWidget(actionSettingsGroup);

    auto *itemsGroup = new QGroupBox("Action Items", m_actionEditor);
    auto *itemsLayout = new QVBoxLayout(itemsGroup);
    m_actionItemList = new QListWidget(itemsGroup);
    m_actionItemList->setObjectName("actionItemList");
    itemsLayout->addWidget(m_actionItemList, 1);

    auto *itemButtons = new QHBoxLayout();
    m_newItemTypeCombo = new QComboBox(itemsGroup);
    m_newItemTypeCombo->addItems(
        {"Launch App",
         "Delay",
         "Press Hotkey",
         "Open Menu",
         "Close Launcher",
         "Custom Script/App (Advanced)",
         "Mouse Move",
         "Mouse Button",
         "Cancel Latest",
         "Cancel Channel",
         "Cancel All",
         "Nth Recent",
         "Nth Frequent"});
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
    actionEditorLayout->addWidget(itemsGroup, 1);

    m_itemDetailStack = new QStackedWidget(m_actionEditor);
    m_itemDetailStack->setObjectName("itemDetailStack");
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
    m_delaySpin->setRange(0, 3600000);
    delayForm->addRow("Milliseconds", m_delaySpin);

    m_itemClosePage = new QWidget(m_itemDetailStack);
    auto *closeLayout = new QVBoxLayout(m_itemClosePage);
    closeLayout->addWidget(new QLabel("Close has no extra settings.", m_itemClosePage));
    closeLayout->addStretch();

    m_itemMenuPage = new QWidget(m_itemDetailStack);
    auto *menuTargetForm = new QFormLayout(m_itemMenuPage);
    m_menuTargetCombo = new QComboBox(m_itemMenuPage);
    menuTargetForm->addRow("Target Menu", m_menuTargetCombo);

    m_itemHotkeyPage = new QWidget(m_itemDetailStack);
    auto *hotkeyForm = new QFormLayout(m_itemHotkeyPage);
    auto *hotkeyModifiers = new QWidget(m_itemHotkeyPage);
    auto *hotkeyModifiersLayout = new QHBoxLayout(hotkeyModifiers);
    hotkeyModifiersLayout->setContentsMargins(0, 0, 0, 0);
    hotkeyModifiersLayout->setSpacing(8);
    m_hotkeyCtrlCheck = new QCheckBox("Ctrl", hotkeyModifiers);
    m_hotkeyAltCheck = new QCheckBox("Alt", hotkeyModifiers);
    m_hotkeyShiftCheck = new QCheckBox("Shift", hotkeyModifiers);
    m_hotkeyWinCheck = new QCheckBox("Win", hotkeyModifiers);
    hotkeyModifiersLayout->addWidget(m_hotkeyCtrlCheck);
    hotkeyModifiersLayout->addWidget(m_hotkeyAltCheck);
    hotkeyModifiersLayout->addWidget(m_hotkeyShiftCheck);
    hotkeyModifiersLayout->addWidget(m_hotkeyWinCheck);
    hotkeyModifiersLayout->addStretch();
    m_hotkeyKeyCombo = new QComboBox(m_itemHotkeyPage);
    populateHotkeyKeyCombo();
    m_hotkeyHoldSpin = new QDoubleSpinBox(m_itemHotkeyPage);
    m_hotkeyHoldSpin->setRange(0.0, 10.0);
    m_hotkeyHoldSpin->setDecimals(2);
    m_hotkeyHoldSpin->setSingleStep(0.1);
    m_hotkeyHoldSpin->setSuffix(" s");
    m_hotkeyProceedCheck = new QCheckBox("Continue immediately", m_itemHotkeyPage);
    hotkeyForm->addRow("Modifiers", hotkeyModifiers);
    hotkeyForm->addRow("Main Key", m_hotkeyKeyCombo);
    hotkeyForm->addRow("Hold Time", m_hotkeyHoldSpin);
    hotkeyForm->addRow("", m_hotkeyProceedCheck);

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
    m_mouseButtonCombo = new QComboBox(m_itemMouseButtonPage);
    m_mouseButtonCombo->addItem("Left", 0);
    m_mouseButtonCombo->addItem("Right", 1);
    m_mouseButtonCombo->addItem("Middle", 2);
    m_mouseButtonActionCombo = new QComboBox(m_itemMouseButtonPage);
    m_mouseButtonActionCombo->addItem("Press", true);
    m_mouseButtonActionCombo->addItem("Release", false);
    mouseButtonForm->addRow("Button", m_mouseButtonCombo);
    mouseButtonForm->addRow("Action", m_mouseButtonActionCombo);

    m_itemCancelPage = new QWidget(m_itemDetailStack);
    auto *cancelForm = new QFormLayout(m_itemCancelPage);
    m_cancelLevelCombo = new QComboBox(m_itemCancelPage);
    m_cancelLevelCombo->addItem("Latest on Channel", static_cast<int>(CancelLevel::Latest));
    m_cancelLevelCombo->addItem("Entire Channel", static_cast<int>(CancelLevel::Channel));
    m_cancelLevelCombo->addItem("All Actions", static_cast<int>(CancelLevel::All));
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
    nthForm->addRow("N (1 = top)", m_nthSpin);
    nthForm->addRow(m_nthHelpLabel);

    m_itemDetailStack->addWidget(m_itemNonePage);
    m_itemDetailStack->addWidget(m_itemLaunchAppPage);
    m_itemDetailStack->addWidget(m_itemScriptPage);
    m_itemDetailStack->addWidget(m_itemDelayPage);
    m_itemDetailStack->addWidget(m_itemClosePage);
    m_itemDetailStack->addWidget(m_itemMenuPage);
    m_itemDetailStack->addWidget(m_itemHotkeyPage);
    m_itemDetailStack->addWidget(m_itemMouseMovePage);
    m_itemDetailStack->addWidget(m_itemMouseButtonPage);
    m_itemDetailStack->addWidget(m_itemCancelPage);
    m_itemDetailStack->addWidget(m_itemNthPage);
    auto *detailGroup = new QGroupBox("Item Details", m_actionEditor);
    auto *detailLayout = new QVBoxLayout(detailGroup);
    detailLayout->addWidget(m_itemDetailStack);
    actionEditorLayout->addWidget(detailGroup);

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
                emit saveRequested(); });

    connect(m_hotkeyRecordButton, &QPushButton::clicked, this, [this]()
            {
                if (!m_isRecordingHotkey) {
                    m_isRecordingHotkey = true;
                    m_hotkeyRecordButton->setText("Press any key combination...");
                    m_hotkeyRecordButton->grabKeyboard();
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
                Menu menu(nullptr, false, false, "New Menu", {}, makeUniqueMenuId("New Menu"));
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
                const std::string removedMenuId = m_menus[index].getId();
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
                const std::string removedActionId = m_actions[index].getId();
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
                    m_menus[index].executeOnRelease = checked;
                } });
    connect(m_exitOnActionCheck, &QCheckBox::toggled, this, [this](bool checked)
            {
                const int index = currentMenuIndex();
                if (index >= 0)
                {
                    m_menus[index].exitOnAction = checked;
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
                m_menus[menuIndex].addActionId(-1, m_actions.front().getId());
                refreshMenuEditor(); });
    connect(removeSlotButton, &QPushButton::clicked, this, [this]()
            {
                const int menuIndex = currentMenuIndex();
                const int slotIndex = m_slotList->currentRow();
                if (menuIndex >= 0 && slotIndex >= 0)
                {
                    m_menus[menuIndex].remAction(slotIndex);
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
                if (menuIndex >= 0 && slotIndex >= 0 && slotIndex + 1 < m_menus[menuIndex].numActions())
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
                switch (m_newItemTypeCombo->currentIndex())
                {
                case 0:
                    item = std::make_unique<AI_LaunchApp>("calculator", "");
                    break;
                case 1:
                    item = std::make_unique<AI_Delay>(0);
                    break;
                case 2:
                    item = std::make_unique<AI_Keystroke>('C', 0x0002, 0.0f, false);
                    break;
                case 3:
                    item = std::make_unique<AI_Menu>(m_menus.empty() ? "" : m_menus.front().getId());
                    break;
                case 4:
                    item = std::make_unique<AI_Close>();
                    break;
                case 5:
                    item = std::make_unique<AI_Script>("");
                    break;
                case 6:
                    item = std::make_unique<AI_MouseMove>(0, 0);
                    break;
                case 7:
                    item = std::make_unique<AI_MouseButton>(0, true);
                    break;
                case 8:
                    item = std::make_unique<AI_Cancel>(CancelLevel::Latest, 0);
                    break;
                case 9:
                    item = std::make_unique<AI_Cancel>(CancelLevel::Channel, 0);
                    break;
                case 10:
                    item = std::make_unique<AI_Cancel>(CancelLevel::All);
                    break;
                case 11:
                    item = std::make_unique<AI_nthRecent>(1);
                    break;
                case 12:
                    item = std::make_unique<AI_nthFrequent>(1);
                    break;
                default:
                    return;
                }
                m_actions[actionIndex].addItem(-1, std::move(item));
                refreshActionEditor();
                m_actionItemList->setCurrentRow(m_actions[actionIndex].len() - 1); });
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
                if (actionIndex >= 0 && itemIndex >= 0 && itemIndex + 1 < m_actions[actionIndex].len())
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
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_LaunchApp *>(m_actions[actionIndex].getItem(itemIndex)) : nullptr;
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
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_LaunchApp *>(m_actions[actionIndex].getItem(itemIndex)) : nullptr;
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
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_LaunchApp *>(m_actions[actionIndex].getItem(itemIndex)) : nullptr;
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
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_Script *>(m_actions[actionIndex].getItem(itemIndex)) : nullptr;
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
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_Script *>(m_actions[actionIndex].getItem(itemIndex)) : nullptr;
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
    auto hotkeyEditorChanged = [this]()
    {
        const int actionIndex = currentActionIndex();
        const int itemIndex = currentActionItemIndex();
        auto *item = actionIndex >= 0 ? dynamic_cast<AI_Keystroke *>(m_actions[actionIndex].getItem(itemIndex)) : nullptr;
        if (item == nullptr)
        {
            return;
        }

        item->modifiers = modifierMask(
            m_hotkeyCtrlCheck->isChecked(),
            m_hotkeyAltCheck->isChecked(),
            m_hotkeyShiftCheck->isChecked(),
            m_hotkeyWinCheck->isChecked());
        item->keycode = m_hotkeyKeyCombo->currentData().toInt();
        item->holdDuration = static_cast<float>(m_hotkeyHoldSpin->value());
        item->proceed = m_hotkeyProceedCheck->isChecked();
        refreshActionItemList();
        refreshActionSummary();
    };
    connect(m_hotkeyCtrlCheck, &QCheckBox::toggled, this, [hotkeyEditorChanged](bool) { hotkeyEditorChanged(); });
    connect(m_hotkeyAltCheck, &QCheckBox::toggled, this, [hotkeyEditorChanged](bool) { hotkeyEditorChanged(); });
    connect(m_hotkeyShiftCheck, &QCheckBox::toggled, this, [hotkeyEditorChanged](bool) { hotkeyEditorChanged(); });
    connect(m_hotkeyWinCheck, &QCheckBox::toggled, this, [hotkeyEditorChanged](bool) { hotkeyEditorChanged(); });
    connect(m_hotkeyKeyCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, hotkeyEditorChanged](int)
            {
                if (!m_isRefreshing)
                {
                    hotkeyEditorChanged();
                } });
    connect(m_hotkeyHoldSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [hotkeyEditorChanged](double) { hotkeyEditorChanged(); });
    connect(m_hotkeyProceedCheck, &QCheckBox::toggled, this, [hotkeyEditorChanged](bool) { hotkeyEditorChanged(); });
    connect(m_mouseMoveXSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value)
            {
                if (m_isRefreshing)
                {
                    return;
                }
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_MouseMove *>(m_actions[actionIndex].getItem(itemIndex)) : nullptr;
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
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_MouseMove *>(m_actions[actionIndex].getItem(itemIndex)) : nullptr;
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
        auto *item = actionIndex >= 0 ? dynamic_cast<AI_MouseButton *>(m_actions[actionIndex].getItem(itemIndex)) : nullptr;
        if (item == nullptr)
        {
            return;
        }
        item->button = m_mouseButtonCombo->currentData().toInt();
        item->down = m_mouseButtonActionCombo->currentData().toBool();
        refreshActionItemList();
        refreshActionSummary();
    };
    connect(m_mouseButtonCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [mouseButtonEditorChanged](int)
            { mouseButtonEditorChanged(); });
    connect(m_mouseButtonActionCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [mouseButtonEditorChanged](int)
            { mouseButtonEditorChanged(); });
    auto cancelEditorChanged = [this]()
    {
        if (m_isRefreshing)
        {
            return;
        }
        const int actionIndex = currentActionIndex();
        const int itemIndex = currentActionItemIndex();
        auto *item = actionIndex >= 0 ? dynamic_cast<AI_Cancel *>(m_actions[actionIndex].getItem(itemIndex)) : nullptr;
        if (item == nullptr)
        {
            return;
        }
        item->level = static_cast<CancelLevel>(m_cancelLevelCombo->currentData().toInt());
        item->channel = static_cast<uint32_t>(m_cancelChannelSpin->value());
        const bool needsChannel = item->level == CancelLevel::Latest || item->level == CancelLevel::Channel;
        m_cancelChannelSpin->setEnabled(needsChannel);
        if (item->level == CancelLevel::Latest)
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
                if (auto *recent = dynamic_cast<AI_nthRecent *>(m_actions[actionIndex].getItem(itemIndex)))
                {
                    recent->n = value;
                }
                else if (auto *frequent = dynamic_cast<AI_nthFrequent *>(m_actions[actionIndex].getItem(itemIndex)))
                {
                    frequent->n = value;
                }
                else
                {
                    return;
                }
                refreshActionItemList();
                refreshActionSummary(); });
    connect(m_delaySpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value)
            {
                const int actionIndex = currentActionIndex();
                const int itemIndex = currentActionItemIndex();
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_Delay *>(m_actions[actionIndex].getItem(itemIndex)) : nullptr;
                if (item != nullptr)
                {
                    item->duration = value;
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
                auto *item = actionIndex >= 0 ? dynamic_cast<AI_Menu *>(m_actions[actionIndex].getItem(itemIndex)) : nullptr;
                if (item != nullptr && comboIndex >= 0)
                {
                    item->menuId = m_menuTargetCombo->itemData(comboIndex).toString().toStdString();
                    refreshActionItemList();
                    refreshActionSummary();
                } });

    m_editorStack->setCurrentWidget(m_emptyEditor);
    m_itemDetailStack->setCurrentWidget(m_itemNonePage);
}

void SettingsWindow::updateHotkeyButtonText()
{
    QString mods;
    if (m_appConfig.globalHotkeyMod & 0x0002) mods += "Ctrl + ";
    if (m_appConfig.globalHotkeyMod & 0x0008) mods += "Win + ";
    if (m_appConfig.globalHotkeyMod & 0x0001) mods += "Alt + ";
    if (m_appConfig.globalHotkeyMod & 0x0004) mods += "Shift + ";
    
    m_hotkeyRecordButton->setText(mods + keyDisplayName(m_appConfig.globalHotkeyVk));
}

bool SettingsWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (m_isRecordingHotkey && event->type() == QEvent::KeyPress)
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

        m_appConfig.globalHotkeyVk = keyEvent->nativeVirtualKey();
        m_appConfig.globalHotkeyMod = 0;
        
        Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
        if (modifiers & Qt::ControlModifier) m_appConfig.globalHotkeyMod |= 0x0002;
        if (modifiers & Qt::AltModifier)     m_appConfig.globalHotkeyMod |= 0x0001;
        if (modifiers & Qt::ShiftModifier)   m_appConfig.globalHotkeyMod |= 0x0004;
        if (modifiers & Qt::MetaModifier)    m_appConfig.globalHotkeyMod |= 0x0008;

        m_isRecordingHotkey = false;
        m_hotkeyRecordButton->releaseKeyboard();
        updateHotkeyButtonText();
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void SettingsWindow::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
    emit windowClosed();
}

void SettingsWindow::loadWorkingCopy(const AppConfig &appConfig, const std::vector<Action> &actions, const std::vector<Menu> &menus)
{
    m_appConfig = appConfig;
    m_actions = actions;
    m_menus = menus;
    m_selectionKind = SelectionKind::None;
    
    updateHotkeyButtonText();
    
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
        m_menuList->addItem(QString::fromStdString(menu.getName()));
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
        QString label = QString::fromStdString(action.getName());
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
    m_menuNameEdit->setText(QString::fromStdString(m_menus[index].getName()));
    m_executeOnReleaseCheck->setChecked(m_menus[index].executeOnRelease);
    m_exitOnActionCheck->setChecked(m_menus[index].exitOnAction);

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
    const QSignalBlocker channelBlocker(m_actionChannelSpin);
    m_actionNameEdit->setText(QString::fromStdString(m_actions[index].getName()));
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
    const std::vector<std::string> &actionIds = m_menus[menuIndex].getActionIds();
    for (const std::string &actionId : actionIds)
    {
        QString label = "Missing Action";
        for (const Action &action : m_actions)
        {
            if (action.getId() == actionId)
            {
                label = QString::fromStdString(action.getName());
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
    for (const auto &item : m_actions[actionIndex].getItems())
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
    for (const auto &item : m_actions[actionIndex].getItems())
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

void SettingsWindow::refreshSlotActionCombo()
{
    QSignalBlocker blocker(m_slotActionCombo);
    m_isRefreshing = true;
    m_slotActionCombo->clear();
    for (const Action &action : m_actions)
    {
        m_slotActionCombo->addItem(QString::fromStdString(action.getName()), QString::fromStdString(action.getId()));
    }

    const int menuIndex = currentMenuIndex();
    const int slotIndex = m_slotList->currentRow();
    if (menuIndex >= 0 && slotIndex >= 0)
    {
        const QString currentId = QString::fromStdString(m_menus[menuIndex].getActionId(slotIndex));
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
        m_menuTargetCombo->addItem(QString::fromStdString(menu.getName()), QString::fromStdString(menu.getId()));
    }

    const int actionIndex = currentActionIndex();
    const int itemIndex = currentActionItemIndex();
    auto *item = actionIndex >= 0 ? dynamic_cast<AI_Menu *>(m_actions[actionIndex].getItem(itemIndex)) : nullptr;
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

    ActionItem *item = m_actions[actionIndex].getItem(itemIndex);
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
        m_delaySpin->setValue(delay->duration);
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

    if (auto *hotkey = dynamic_cast<AI_Keystroke *>(item))
    {
        const QSignalBlocker ctrlBlocker(m_hotkeyCtrlCheck);
        const QSignalBlocker altBlocker(m_hotkeyAltCheck);
        const QSignalBlocker shiftBlocker(m_hotkeyShiftCheck);
        const QSignalBlocker winBlocker(m_hotkeyWinCheck);
        const QSignalBlocker keyBlocker(m_hotkeyKeyCombo);
        const QSignalBlocker holdBlocker(m_hotkeyHoldSpin);
        const QSignalBlocker proceedBlocker(m_hotkeyProceedCheck);
        m_hotkeyCtrlCheck->setChecked((hotkey->modifiers & 0x0002) != 0);
        m_hotkeyAltCheck->setChecked((hotkey->modifiers & 0x0001) != 0);
        m_hotkeyShiftCheck->setChecked((hotkey->modifiers & 0x0004) != 0);
        m_hotkeyWinCheck->setChecked((hotkey->modifiers & 0x0008) != 0);
        const int comboIndex = m_hotkeyKeyCombo->findData(hotkey->keycode);
        m_hotkeyKeyCombo->setCurrentIndex(comboIndex >= 0 ? comboIndex : 0);
        m_hotkeyHoldSpin->setValue(hotkey->holdDuration);
        m_hotkeyProceedCheck->setChecked(hotkey->proceed);
        m_itemDetailStack->setCurrentWidget(m_itemHotkeyPage);
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
        const QSignalBlocker actionBlocker(m_mouseButtonActionCombo);
        const int buttonIndex = m_mouseButtonCombo->findData(mouseButton->button);
        m_mouseButtonCombo->setCurrentIndex(buttonIndex >= 0 ? buttonIndex : 0);
        const int actionIndexCombo = m_mouseButtonActionCombo->findData(mouseButton->down);
        m_mouseButtonActionCombo->setCurrentIndex(actionIndexCombo >= 0 ? actionIndexCombo : 0);
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
        const bool needsChannel = cancel->level == CancelLevel::Latest || cancel->level == CancelLevel::Channel;
        m_cancelChannelSpin->setEnabled(needsChannel);
        if (cancel->level == CancelLevel::Latest)
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

    if (auto *recent = dynamic_cast<AI_nthRecent *>(item))
    {
        const QSignalBlocker blocker(m_nthSpin);
        m_nthSpin->setValue(std::max(1, recent->n));
        m_nthHelpLabel->setText("Runs the Nth most recently launched action from the wheel (1 = most recent).");
        m_itemDetailStack->setCurrentWidget(m_itemNthPage);
        return;
    }

    if (auto *frequent = dynamic_cast<AI_nthFrequent *>(item))
    {
        const QSignalBlocker blocker(m_nthSpin);
        m_nthSpin->setValue(std::max(1, frequent->n));
        m_nthHelpLabel->setText("Runs the Nth most frequently launched action from the wheel (1 = most used).");
        m_itemDetailStack->setCurrentWidget(m_itemNthPage);
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
        return QString("Delay: %1 ms").arg(static_cast<const AI_Delay *>(item)->duration);
    case ActionItemKind::Keystroke:
    {
        const auto *hotkey = static_cast<const AI_Keystroke *>(item);
        QStringList keys;
        if ((hotkey->modifiers & 0x0002) != 0)
            keys << "Ctrl";
        if ((hotkey->modifiers & 0x0001) != 0)
            keys << "Alt";
        if ((hotkey->modifiers & 0x0004) != 0)
            keys << "Shift";
        if ((hotkey->modifiers & 0x0008) != 0)
            keys << "Win";
        if (hotkey->keycode != 0)
            keys << keyDisplayName(hotkey->keycode);
        return keys.isEmpty() ? "Press Hotkey: choose keys" : QString("Press Hotkey: %1").arg(keys.join("+"));
    }
    case ActionItemKind::Menu:
    {
        const auto *menuItem = static_cast<const AI_Menu *>(item);
        for (const Menu &menu : m_menus)
        {
            if (menu.getId() == menuItem->menuId)
            {
                return QString("Open Menu: %1").arg(QString::fromStdString(menu.getName()));
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
        return QString("Mouse Button: %1 %2")
            .arg(name)
            .arg(button->down ? "Press" : "Release");
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
        case CancelLevel::Latest:
        default:
            if (cancel->channel == 0)
            {
                return "Cancel Latest (any channel)";
            }
            return QString("Cancel Latest: channel %1").arg(cancel->channel);
        }
    }
    case ActionItemKind::NthRecent:
        return QString("Nth Recent: %1").arg(static_cast<const AI_nthRecent *>(item)->n);
    case ActionItemKind::NthFrequent:
        return QString("Nth Frequent: %1").arg(static_cast<const AI_nthFrequent *>(item)->n);
    case ActionItemKind::KeyRelease:
    {
        const auto *release = static_cast<const AI_KeyRelease *>(item);
        return QString("Key Release: %1").arg(keyDisplayName(release->keycode));
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
    if (vk >= 0x70 && vk <= 0x7B)
    {
        return QString("F%1").arg(vk - 0x6F);
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

void SettingsWindow::populateHotkeyKeyCombo()
{
    if (m_hotkeyKeyCombo == nullptr)
    {
        return;
    }

    m_hotkeyKeyCombo->clear();
    for (int key = 'A'; key <= 'Z'; ++key)
    {
        m_hotkeyKeyCombo->addItem(QString(QChar(key)), key);
    }
    for (int key = '0'; key <= '9'; ++key)
    {
        m_hotkeyKeyCombo->addItem(QString(QChar(key)), key);
    }

    const std::vector<int> extraKeys = {
        0x0D, 0x09, 0x1B, 0x20, 0x25, 0x26, 0x27, 0x28, 0x08, 0x2E,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75,
        0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B};
    for (int key : extraKeys)
    {
        m_hotkeyKeyCombo->addItem(keyDisplayName(key), key);
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
        for (const std::string &slotActionId : menu.getActionIds())
        {
            if (slotActionId != actionId)
            {
                keptIds.push_back(slotActionId);
            }
        }

        Menu replacement(nullptr, menu.executeOnRelease, menu.exitOnAction, menu.getName(), keptIds, menu.getId());
        menu = replacement;
    }
}

void SettingsWindow::clearMenuReferences(const std::string &menuId)
{
    for (Action &action : m_actions)
    {
        for (const auto &itemPtr : action.getItems())
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
            if (action.getId() == candidate.toStdString())
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
            if (menu.getId() == candidate.toStdString())
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
        if (action.getId().empty())
        {
            errorMessage = "Every action must have a non-empty ID.";
            return false;
        }
        if (std::find(actionIds.begin(), actionIds.end(), action.getId()) != actionIds.end())
        {
            errorMessage = "Action IDs must be unique.";
            return false;
        }
        actionIds.push_back(action.getId());

        for (const auto &itemPtr : action.getItems())
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
                    errorMessage = QString("Advanced script/app item in '%1' has an empty path.").arg(QString::fromStdString(action.getName()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::LaunchApp)
            {
                const auto *launch = static_cast<const AI_LaunchApp *>(itemPtr.get());
                if (launch->presetId.empty())
                {
                    errorMessage = QString("Launch App item in '%1' is missing its preset.").arg(QString::fromStdString(action.getName()));
                    return false;
                }
                if (launch->presetId == "custom" && QString::fromStdString(launch->customTarget).trimmed().isEmpty())
                {
                    errorMessage = QString("Launch App item in '%1' needs a custom app path.").arg(QString::fromStdString(action.getName()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::Delay)
            {
                const auto *delay = static_cast<const AI_Delay *>(itemPtr.get());
                if (delay->duration < 0)
                {
                    errorMessage = QString("Delay item in '%1' must be non-negative.").arg(QString::fromStdString(action.getName()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::Keystroke)
            {
                const auto *hotkey = static_cast<const AI_Keystroke *>(itemPtr.get());
                if (hotkey->keycode == 0)
                {
                    errorMessage = QString("Press Hotkey item in '%1' needs a main key.").arg(QString::fromStdString(action.getName()));
                    return false;
                }
                if (hotkey->holdDuration < 0.0f)
                {
                    errorMessage = QString("Press Hotkey item in '%1' has an invalid hold time.").arg(QString::fromStdString(action.getName()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::Menu)
            {
                const auto *menuItem = static_cast<const AI_Menu *>(itemPtr.get());
                bool foundMenu = false;
                for (const Menu &menu : m_menus)
                {
                    if (menu.getId() == menuItem->menuId)
                    {
                        foundMenu = true;
                        break;
                    }
                }
                if (!foundMenu)
                {
                    errorMessage = QString("Menu action '%1' targets a missing menu.").arg(QString::fromStdString(action.getName()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::MouseButton)
            {
                const auto *button = static_cast<const AI_MouseButton *>(itemPtr.get());
                if (button->button < 0 || button->button > 2)
                {
                    errorMessage = QString("Mouse Button item in '%1' has an invalid button.").arg(QString::fromStdString(action.getName()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::NthRecent)
            {
                if (static_cast<const AI_nthRecent *>(itemPtr.get())->n < 1)
                {
                    errorMessage = QString("Nth Recent item in '%1' must be >= 1.").arg(QString::fromStdString(action.getName()));
                    return false;
                }
            }
            else if (itemPtr->kind() == ActionItemKind::NthFrequent)
            {
                if (static_cast<const AI_nthFrequent *>(itemPtr.get())->n < 1)
                {
                    errorMessage = QString("Nth Frequent item in '%1' must be >= 1.").arg(QString::fromStdString(action.getName()));
                    return false;
                }
            }
        }
    }

    std::vector<std::string> menuIds;
    for (const Menu &menu : m_menus)
    {
        if (menu.getId().empty())
        {
            errorMessage = "Every menu must have a non-empty ID.";
            return false;
        }
        if (std::find(menuIds.begin(), menuIds.end(), menu.getId()) != menuIds.end())
        {
            errorMessage = "Menu IDs must be unique.";
            return false;
        }
        menuIds.push_back(menu.getId());

        for (const std::string &actionId : menu.getActionIds())
        {
            if (std::find(actionIds.begin(), actionIds.end(), actionId) == actionIds.end())
            {
                errorMessage = QString("Menu '%1' references a missing action.").arg(QString::fromStdString(menu.getName()));
                return false;
            }
        }
    }

    return true;
}
