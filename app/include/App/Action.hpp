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
#include <memory>
#include <vector>
#include <Platform/Inputs.hpp>
#include <string>
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
               std::string iconFilepath = "");
        Action(const Action &other);
        Action &operator=(const Action &other);
        Action(Action &&other) noexcept;
        Action &operator=(Action &&other) noexcept;
        ~Action();

        /**
         * @brief Add an item from the sequence
         *
         * @param ind Index of item to add
         * @param ai Action item to add
         */
        void addItem(int ind, std::unique_ptr<ActionItem> ai);

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
        bool isScriptAction() const;
        std::string getScriptPath() const;
        void setName(const std::string &newName);
        void setScriptAction(const std::string &path);

    private:
        std::vector<std::unique_ptr<ActionItem>> sequence;
        std::string name;
        std::string iconFilepath;
    };
}
