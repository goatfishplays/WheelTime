/**
 * @file SettingsWindow.hpp
 * @brief Settings editor for menus and reusable actions.
 */

#pragma once

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QWidget>

#include <vector>

#include "App/App.hpp"
#include "App/Action.hpp"
#include "App/ActionItems/Cancel.hpp"
#include "App/Menu.hpp"

namespace Application
{
    /**
     * @brief Non-modal editor for the shared action library and wheel menus.
     *
     * The window edits a working copy rather than mutating the live runtime
     * objects directly. On save, the working copy is exported back to `App`,
     * validated, written to JSON, and then applied to the active launcher state.
     */
    class SettingsWindow : public QWidget
    {
        Q_OBJECT

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;

    public:
        explicit SettingsWindow(QWidget *parent = nullptr);

        /// @brief Initializes the editor with a fresh working copy of runtime data.
        void loadWorkingCopy(const AppConfig &appConfig, const std::vector<Action> &actions, const std::vector<Menu> &menus);
        /// @brief Exports the edited working copy back to the caller.
        void exportWorkingCopy(AppConfig &appConfig, std::vector<Action> &actions, std::vector<Menu> &menus) const;

    signals:
        /// @brief Emitted after the local working copy passes UI-level validation.
        void saveRequested();
        /// @brief Emitted when the settings window is closed (pause/resume hook).
        void windowClosed();

    protected:
        void closeEvent(QCloseEvent *event) override;

    private:
        /// @brief Tracks which top-level entity is currently shown in the right pane.
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
        void refreshActionIconPreview();
        void refreshSlotActionCombo();
        void refreshMenuTargetCombo();
        void refreshItemDetail();
        void updateHotkeyButtonText();
        void updateCancelChannelVisibility(CancelLevel level);
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

        AppConfig m_appConfig;
        /// @brief Working-copy action library edited by the settings UI.
        std::vector<Action> m_actions;
        /// @brief Working-copy menus edited by the settings UI.
        std::vector<Menu> m_menus;
        bool m_isRefreshing{false};
        SelectionKind m_selectionKind{SelectionKind::None};
        bool m_isRecordingHotkey{false};

        QListWidget *m_menuList{nullptr};
        QListWidget *m_actionList{nullptr};
        QStackedWidget *m_editorStack{nullptr};

        QWidget *m_emptyEditor{nullptr};
        QWidget *m_menuEditor{nullptr};
        QWidget *m_actionEditor{nullptr};

        QPushButton *m_hotkeyRecordButton{nullptr};
        QPushButton *m_hotkeyClearButton{nullptr};

        QGroupBox *m_globalGroup{nullptr};
        QCheckBox *m_darkModeCheck{nullptr};

        QLineEdit *m_menuNameEdit{nullptr};
        QCheckBox *m_executeOnReleaseCheck{nullptr};
        QCheckBox *m_exitOnActionCheck{nullptr};
        QCheckBox *m_centerMouseOnOpenCheck{nullptr};
        QCheckBox *m_restoreMouseOnCloseCheck{nullptr};
        QListWidget *m_slotList{nullptr};
        QComboBox *m_slotActionCombo{nullptr};

        QLineEdit *m_actionNameEdit{nullptr};
        QLineEdit *m_actionIconEdit{nullptr};
        QPushButton *m_browseActionIconButton{nullptr};
        QPushButton *m_clearActionIconButton{nullptr};
        QLabel *m_actionIconPreview{nullptr};
        QSpinBox *m_actionChannelSpin{nullptr};
        QLabel *m_actionChannelHelpLabel{nullptr};
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
        QWidget *m_itemMouseMovePage{nullptr};
        QWidget *m_itemMouseButtonPage{nullptr};
        QWidget *m_itemCancelPage{nullptr};
        QWidget *m_itemNthPage{nullptr};
        QWidget *m_itemSocketPage{nullptr};
        QWidget *m_itemSearchPage{nullptr};
        QWidget *m_itemKeyReleasePage{nullptr};
        QLabel *m_keyReleaseHelpLabel{nullptr};
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
        QSpinBox *m_mouseMoveXSpin{nullptr};
        QSpinBox *m_mouseMoveYSpin{nullptr};
        QComboBox *m_mouseButtonCombo{nullptr};
        QCheckBox *m_mouseButtonCtrlCheck{nullptr};
        QCheckBox *m_mouseButtonAltCheck{nullptr};
        QCheckBox *m_mouseButtonShiftCheck{nullptr};
        QCheckBox *m_mouseButtonWinCheck{nullptr};
        QDoubleSpinBox *m_mouseButtonHoldSpin{nullptr};
        QCheckBox *m_mouseButtonProceedCheck{nullptr};
        QComboBox *m_cancelLevelCombo{nullptr};
        QSpinBox *m_cancelChannelSpin{nullptr};
        QLabel *m_cancelHelpLabel{nullptr};
        QSpinBox *m_nthSpin{nullptr};
        QLabel *m_nthHelpLabel{nullptr};
        QPushButton *m_resetFrequenciesButton{nullptr};
        QComboBox *m_socketProtocolCombo{nullptr};
        QLineEdit *m_socketDestinationEdit{nullptr};
        QLineEdit *m_socketMessageEdit{nullptr};
        QLineEdit *m_socketHttpMethodEdit{nullptr};
        QLabel *m_socketHelpLabel{nullptr};
        QCheckBox *m_searchActionsCheck{nullptr};
        QCheckBox *m_searchProgramsCheck{nullptr};
        QCheckBox *m_searchMenusCheck{nullptr};
        QCheckBox *m_searchWebCheck{nullptr};
        QLineEdit *m_searchWebUrlEdit{nullptr};
        QLabel *m_searchHelpLabel{nullptr};
        QLabel *m_actionSequenceLabel{nullptr};
    };
}
