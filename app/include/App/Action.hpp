/**
 * @file Action.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once
#include <vector>
#include <Platform/Inputs.hpp>
#include "App/ActionItems.hpp"

namespace Application
{
    /**
     * @brief Contains a sequence of actions to be executed
     *
     */
    class Action
    {
    public:
        /**
         * @brief Add an item from the sequence
         *
         * @param ind Index of item to add
         * @param ai Action item to add
         */
        void addItem(int ind, ActionItem &ai);

        /**
         * @brief Remove an item from the sequence
         *
         * @param ind Index of item to remove
         */
        void removeItem(int ind);

        /**
         * @brief Gets the number of `ActionItem`s in the sequence
         *
         * @return int
         */
        int len();

        /**
         * @brief Runs the action items in sequence
         *
         */
        void execute();

    private:
        std::vector<ActionItem> sequence;
    };
}