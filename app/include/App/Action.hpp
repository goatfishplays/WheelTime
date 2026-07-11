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
#include <string>
#include <memory>
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
        Action(std::vector<std::unique_ptr<ActionItem>> sequence = {},
               std::string name = "Unnamed Action",
               std::string iconFilepath = "", uint32_t channel = 0);
        ~Action();

        /**
         * @brief Add an item from the sequence
         *
         * @param ind Index of item to add
         * @param ai Action item to add
         */
        void addItem(int ind, ActionItem ai);

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

        std::string getName() const;

        /**
         * @brief Returns the channel this action will run on
         *
         * @return uint32_t
         */
        uint32_t channel() const;

        /**
         * @brief Returns a constant reference to the actionItems
         *
         * @return const std::vector<std::unique_ptr<ActionItem>>&
         */
        const std::vector<std::unique_ptr<ActionItem>> &items() const;

    private:
        std::vector<std::unique_ptr<ActionItem>> sequence;
        std::string name;
        std::string iconFilepath;

        uint32_t m_channel;
    };
}