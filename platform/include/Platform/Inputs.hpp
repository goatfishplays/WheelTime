/**
 * @file Inputs.hpp
 * @author goatfishplays@gmail.com
 * @brief Allows grabbing of inputs such as mouse position or keystrokes
 * @version 0.1
 * @date 2026-06-29
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <memory>
#include <string>

namespace Platform
{
    // TODO: Come up with legend for special keys, mouse buttons, midi, etc
    struct InputBind
    {
        int input;
        int mod;
    };
    struct Vec2
    {
        int x;
        int y;
    };

    /**
     * @brief OS agnostic input receiver
     *
     */
    class InputRcvr
    {
    public:
        InputRcvr();
        ~InputRcvr();
        // TODO: Oh gosh I just realized multimonitor exists, I have no idea how that works, good luck :)
        /**
         * @brief Get mouse position relative to the monitor
         *
         * Note: Position is x: left to right, y: top to bottom
         *
         */
        Vec2 getAbsoluteMousePosition();

        /**
         * @brief Get mouse position relative to window,
         *
         * Note: Position is x: left to right, y: top to bottom
         *
         */
        Vec2 getRelativeMousePosition();

        /**
         * @brief Registers an input to be tracked
         *
         * Currently only aim to support keyboard inputs
         *
         */
        void registerInputBinding(InputBind);

        /**
         * @brief Unregisters an input being tracked
         *
         * Currently only aim to support keyboard inputs
         *
         */
        void unregisterInputBinding(InputBind);

    private:
        /**
         * @brief The actual implementation detail
         *
         */
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
}