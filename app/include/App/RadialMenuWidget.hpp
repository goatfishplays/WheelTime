#pragma once
#include "App/Action.hpp"
#include "App/HoverButton.hpp"
#include "App/Menu.hpp"

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include <QWidget>
#include <QLabel>
#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QResizeEvent>
#include <QEnterEvent>
#include <QMouseEvent>
#include <QKeyEvent>

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
         * @brief Set the menu model and visible slot visuals, then rebuild the wheel.
         *
         * `menu` still supplies slot count and selection behavior, while
         * `slotVisuals` lets the wheel render names/icons resolved from the shared
         * action library rather than assuming the menu owns full `Action`s.
         *
         * @param menu Active menu model
         * @param slotVisuals Display label + optional icon path for each slot
         */
        void setMenu(const Menu &menu, const std::vector<ActionSlotVisual> &slotVisuals);
        /**
         * @brief Set the Center Text object
         *
         * @param text
         */
        void setCenterText(const QString &text);
        /**
         * @brief Set the button ring radius as a fraction of the shorter widget side.
         *
         * Values are clamped to a usable range; the effective pixel radius is
         * recomputed on resize so the wheel scales with the monitor/panel size.
         *
         * @param fraction Typically ~0.25–0.40 (default 0.32)
         */
        void setButtonRadiusFraction(double fraction);
        /**
         * @brief Set a fixed pixel radius (disables size-based scaling until
         * setButtonRadiusFraction is called again).
         *
         * @param radius Absolute distance from center to button centers
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

    private:
        void rebuildButtons();
        void repositionButtons();
        void updateSelection();
        void clearSelection();
        int indexFromAngle(double angleRadians, int count) const;
        /// @brief Floating geometric center of this widget (same basis for layout + hit-testing).
        [[nodiscard]] QPointF layoutCenter() const;
        /// @brief Floating geometric center of a child in this widget's coordinates.
        [[nodiscard]] static QPointF widgetCenter(const QWidget *widget);
        /// @brief Move @p widget so its geometric center sits on @p center.
        static void placeWidgetCentered(QWidget *widget, QPointF center);
        /// @brief Pixel ring radius for the current widget size / mode.
        [[nodiscard]] int effectiveButtonRadius() const;
        /// @brief Cached QIcon for @p path (empty path → null icon).
        [[nodiscard]] QIcon iconForPath(const std::string &path) const;

        Menu m_menu;
        std::vector<ActionSlotVisual> m_slotVisuals;
        QPoint m_mousePosition;
        QLabel *m_centerLabel{nullptr};
        std::vector<HoverButton *> m_buttons;
        /// @brief Avoid re-decoding the same icon file on every menu rebuild/open.
        mutable std::unordered_map<std::string, QIcon> m_iconCache;

        int m_buttonRadius{220};
        double m_buttonRadiusFraction{0.32};
        bool m_useFixedButtonRadius{false};
        ActivationMode m_mode{ActivationMode::Exact};
        double m_maxDistance{-1.0};
        int m_selectedIndex{-1};
    };
}
