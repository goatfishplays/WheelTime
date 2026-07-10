#pragma once
#include "App/HoverButton.hpp"
#include "App/Menu.hpp"

#include <vector>
#include <string>
#include <memory>

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QResizeEvent>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPushButton>

namespace Application
{

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

        /**
         * @brief Set the Menu object and updates radial wheel accordingly
         *
         * @param menu
         */
        void setMenu(const Menu &menu, const std::vector<std::string> &actionLabels);
        /**
         * @brief Set the Center Text object
         *
         * @param text
         */
        void setCenterText(const QString &text);
        /**
         * @brief Set the distance of buttons from the center
         *
         * @param radius
         */
        void setButtonRadius(int radius);
        /**
         * @brief Set the Activation mode
         * TODO: closest Angle can probs be removed and just use closest distance
         * @param mode
         */
        void setActivationMode(ActivationMode mode);
        /**
         * @brief Set the max radius a button will be considered for (selection set to -1 if out of all radii)
         *
         * @param maxDistance max distance from button to consider, negative for no limit
         */
        void setMaxDistance(double maxDistance);
        /**
         * @brief Get the index of the selected action
         *
         * @return int
         */
        int getSelectedIndex() const;
        /**
         * @brief Set the index of the selected action
         *
         * @param newVal
         */
        void setSelectedIndex(int newVal);
        /**
         * @brief updates the selection based on the global mouse position
         *
         * @param globalPos
         */
        void updateSelectionFromGlobalMousePosition(const QPoint &globalPos);

    signals:
        void selectedIndexChanged(int index);
        void buttonTriggered(int index);

    protected:
        void resizeEvent(QResizeEvent *event) override;
        bool eventFilter(QObject *watched, QEvent *event) override;
        // void mouseMoveEvent(QMouseEvent *event) override;

    private:
        /**
         * @brief Reconfigure and rerender menu after changes such as setting new menu or etc.
         *
         */
        void rebuildButtons();
        /**
         * @brief rebuild the wheel
         *
         */
        void repositionButtons();
        /**
         * @brief detect if update to selection after pos set
         *
         */
        void updateSelection();
        /**
         * @brief Set selection to -1
         *
         */
        void clearSelection();
        /**
         * @brief Gets the selection index associated with a point
         *
         * @param angleRadians Angle of mouse from center
         * @param count Number of segments
         * @return int
         */
        int indexFromAngle(double angleRadians, int count) const;

        Menu m_menu;
        std::vector<std::string> m_actionLabels;
        QPoint m_mousePosition;
        QLabel *m_centerLabel{nullptr};
        std::vector<HoverButton *> m_buttons;

        int m_buttonRadius{110};
        ActivationMode m_mode{ActivationMode::Exact};
        double m_maxDistance{-1.0};
        int m_selectedIndex{-1};
    };
}
