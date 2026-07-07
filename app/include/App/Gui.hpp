#pragma once

#include <vector>
#include <string>
#include <memory>

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QResizeEvent>
#include <QEnterEvent>
#include <QMouseEvent>

#include "App/Menu.hpp"
#include "App/Action.hpp"

namespace Application
{
    // struct Action
    // {
    //     std::string name;
    // };

    // struct Menu
    // {
    //     std::string name;
    //     std::vector<Action> actions;
    // };

    class HoverButton : public QPushButton
    {
        Q_OBJECT
    public:
        explicit HoverButton(QWidget *parent = nullptr);

    signals:
        void mouseHovered();
        void mouseLeft();

    protected:
        void enterEvent(QEnterEvent *event) override;
        void leaveEvent(QEvent *event) override;
    };

    class RadialMenuWidget : public QWidget
    {
        Q_OBJECT

    public:
        enum class ActivationMode
        {
            Distance,
            Angle,
            Exact
        };

        explicit RadialMenuWidget(QWidget *parent = nullptr);
        explicit RadialMenuWidget(const Menu &menu, QWidget *parent = nullptr);

        void setMenu(const Menu &menu);
        void setCenterText(const QString &text);
        void setButtonRadius(int radius);
        void setActivationMode(ActivationMode mode);
        void setMaxDistance(double maxDistance); // negative means "no limit"
        int getSelectedIndex() const;
        void setSelectedIndex(int newVal);
        void setMousePosFromGlobal(const QPoint &globalPos);

    signals:
        void selectedIndexChanged(int index);
        void buttonTriggered(int index);

    protected:
        void resizeEvent(QResizeEvent *event) override;
        bool eventFilter(QObject *watched, QEvent *event) override;
        // void mouseMoveEvent(QMouseEvent *event) override;

    private:
        void rebuildButtons();
        void repositionButtons();
        void updateSelection();
        void clearSelection();
        int indexFromAngle(double angleRadians, int count) const;

        Menu m_menu;
        QPoint m_mousePosition;
        QLabel *m_centerLabel{nullptr};
        std::vector<HoverButton *> m_buttons;

        int m_buttonRadius{110};
        ActivationMode m_mode{ActivationMode::Exact};
        double m_maxDistance{-1.0};
        int m_selectedIndex{-1};
    };

    class Gui : public QWidget
    {
        Q_OBJECT

    public:
        explicit Gui(QWidget *parent = nullptr);

        void onSelectChange(int);

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;

    private:
        QLabel *m_titleLabel{nullptr};
        RadialMenuWidget *m_radialMenu{nullptr};
        QPushButton *m_settingsButton{nullptr};
        QPushButton *m_editButton{nullptr};
    };
}