#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QWidget>

#include <vector>

#include "App/Action.hpp"
#include "App/Menu.hpp"

namespace Application
{
    class SettingsWindow : public QWidget
    {
        Q_OBJECT

    public:
        explicit SettingsWindow(QWidget *parent = nullptr);

        void loadWorkingCopy(const std::vector<Action> &actions, const std::vector<Menu> &menus);
        void exportWorkingCopy(std::vector<Action> &actions, std::vector<Menu> &menus) const;

    signals:
        void saveRequested();

    private:
        enum class SelectionKind
        {
            None,
            Menu,
            Action
        };

        void refreshLists();
        void refreshMenuList();
        void refreshActionList();
        void refreshEditors();
        void refreshMenuEditor();
        void refreshActionEditor();
        void refreshSlotList();
        void refreshActionItemList();
        void refreshActionSummary();
        void refreshSlotActionCombo();
        void refreshMenuTargetCombo();
        void refreshItemDetail();
        QString describeActionItem(const ActionItem *item) const;
        QString launchPresetDisplayName(const std::string &presetId) const;
        QString launchPresetTarget(const std::string &presetId) const;
        QString keyDisplayName(int vk) const;
        void populateLaunchPresetCombo();
        void populateHotkeyKeyCombo();
        void setSelectedMenu(int index);
        void setSelectedAction(int index);
        int currentMenuIndex() const;
        int currentActionIndex() const;
        int currentActionItemIndex() const;
        void deleteActionReferences(const std::string &actionId);
        void clearMenuReferences(const std::string &menuId);
        std::string makeUniqueActionId(const QString &seed) const;
        std::string makeUniqueMenuId(const QString &seed) const;
        bool validateWorkingCopy(QString &errorMessage) const;

        std::vector<Action> m_actions;
        std::vector<Menu> m_menus;
        bool m_isRefreshing{false};
        SelectionKind m_selectionKind{SelectionKind::None};

        QListWidget *m_menuList{nullptr};
        QListWidget *m_actionList{nullptr};
        QStackedWidget *m_editorStack{nullptr};

        QWidget *m_emptyEditor{nullptr};
        QWidget *m_menuEditor{nullptr};
        QWidget *m_actionEditor{nullptr};

        QLineEdit *m_menuNameEdit{nullptr};
        QCheckBox *m_executeOnReleaseCheck{nullptr};
        QCheckBox *m_exitOnActionCheck{nullptr};
        QListWidget *m_slotList{nullptr};
        QComboBox *m_slotActionCombo{nullptr};

        QLineEdit *m_actionNameEdit{nullptr};
        QListWidget *m_actionItemList{nullptr};
        QComboBox *m_newItemTypeCombo{nullptr};
        QStackedWidget *m_itemDetailStack{nullptr};
        QWidget *m_itemNonePage{nullptr};
        QWidget *m_itemLaunchAppPage{nullptr};
        QWidget *m_itemScriptPage{nullptr};
        QWidget *m_itemDelayPage{nullptr};
        QWidget *m_itemClosePage{nullptr};
        QWidget *m_itemMenuPage{nullptr};
        QWidget *m_itemHotkeyPage{nullptr};
        QComboBox *m_launchPresetCombo{nullptr};
        QLineEdit *m_launchCustomPathEdit{nullptr};
        QPushButton *m_browseLaunchAppButton{nullptr};
        QLineEdit *m_scriptPathEdit{nullptr};
        QPushButton *m_browseScriptButton{nullptr};
        QSpinBox *m_delaySpin{nullptr};
        QComboBox *m_menuTargetCombo{nullptr};
        QCheckBox *m_hotkeyCtrlCheck{nullptr};
        QCheckBox *m_hotkeyAltCheck{nullptr};
        QCheckBox *m_hotkeyShiftCheck{nullptr};
        QCheckBox *m_hotkeyWinCheck{nullptr};
        QComboBox *m_hotkeyKeyCombo{nullptr};
        QDoubleSpinBox *m_hotkeyHoldSpin{nullptr};
        QCheckBox *m_hotkeyProceedCheck{nullptr};
        QLabel *m_actionSequenceLabel{nullptr};
    };
}
