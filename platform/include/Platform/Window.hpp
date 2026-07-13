/**
 * @file Window.hpp
 * @author goatfishplays@gmail.com
 * @brief Window controls such as bringing it forth or hiding it once again
 * @version 0.1
 * @date 2026-06-29
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <memory>

namespace Platform
{
    struct WindowRect
    {
        int x{};
        int y{};
        int width{};
        int height{};
    };

    /**
     * @brief OS agnostic window controller
     *
     * TODO: Decide if we want to use QT for the actual GUI
     *
     */
    class Window
    {
    public:
        Window();
        Window(void* nativeHandle);
        ~Window();

        /**
         * @brief Sets this window to the currently active one
         *
         */
        void getActiveWindow();
        /**
         * @brief Bring the window into active view/focus
         *
         */
        void focus();
        /**
         * @brief Minimize/hide the window
         *
         */
        void hide();
        /**
         * @brief Shows the window without activating it.
         *
         * Used by the launcher overlay so keyboard focus stays on the
         * underlying application while the wheel becomes visible.
         */
        void showNoActivate();
        /**
         * @brief Keeps or clears native always-on-top state.
         */
        void setTopmost(bool enabled);
        /**
         * @brief Toggles native no-activate behavior.
         */
        void setNonActivating(bool enabled);
        /**
         * @brief Enables layered transparency required by the overlay shell.
         */
        void setTransparentOverlay(bool enabled);
        /**
         * @brief Toggles whether mouse input passes through to windows below.
         *
         * The dormant launcher overlay uses click-through so it can stay alive
         * without intercepting gameplay or normal desktop interaction.
         */
        void setClickThrough(bool enabled);
        /**
         * @brief Repositions and resizes the native window.
         */
        void setBounds(const WindowRect &bounds);
        /**
         * @brief Finds the monitor bounds containing the supplied screen point.
         */
        WindowRect monitorBoundsForPoint(int x, int y) const;
        /**
         * @brief Adds button at set location to the window
         *
         */

    private:
        /**
         * @brief The actual implementation detail
         *
         */
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
