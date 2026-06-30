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
        ~Window();

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