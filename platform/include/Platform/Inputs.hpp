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
    /**
     * @brief OS agnostic input receiver
     *
     */
    class InputRcvr
    {
    public:
        InputRcvr();
        ~InputRcvr();
        struct VEC2
        {
            int x;
            int y;
        };
        // TODO: Oh gosh I just realized multimonitor exists, I have no idea how that works, good luck :)
        /**
         * @brief Get mouse position relative to the monitor
         *
         * Note: Position is x: left to right, y: top to bottom
         *
         */
        VEC2 getAbsoluteMousePosition();

        /**
         * @brief Get mouse position relative to window,
         *
         * Note: Position is x: left to right, y: top to bottom
         *
         */
        VEC2 getRelativeMousePosition();

        /**
         * @brief Registers an input to be tracked
         *
         * Currently only aim to support keyboard inputs
         *
         * TODO: MIDI and mouse extra buttons and other input support
         *
         * TODO: Come up with legend for special keys and modifiers "<C-KEY>"" perhaps or something or make separate mod arg, would probs be better to handle this with ints or something anyway
         *
         */
        void registerInputBinding(int);

    private:
        /**
         * @brief The actual implementation detail
         *
         */
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
}