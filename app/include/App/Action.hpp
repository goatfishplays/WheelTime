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
#include <string>
#include <cstdint>
#include <Platform/Inputs.hpp>
#include "App/ActionItems.hpp"

namespace Application
{
    /**
     * @brief Represents one reusable macro/action in the shared action library.
     *
     * Each `Action` owns an ordered sequence of polymorphic `ActionItem`s.
     * Menus reference actions by stable ID instead of embedding action copies.
     */
    class Action
    {
    public:
        Action(std::vector<std::unique_ptr<ActionItem>> sequence = {},
               std::string name = "Unnamed Action",
               std::string iconFilepath = "",
               std::string id = "",
               uint32_t channel = 0);
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
        int len() const;

        /**
         * @brief Runs the action items in sequence
         *
         */
        void execute();

        /// @brief Stable config ID used by menus and serialization.
        std::string getId() const;
        std::string getName() const;
        /// @brief Exposes the item sequence for settings editing/serialization.
        const std::vector<std::unique_ptr<ActionItem>> &getItems() const;
        ActionItem *getItem(int index);
        const ActionItem *getItem(int index) const;
        void setName(const std::string &newName);
        void setId(const std::string &newId);
        /// @brief Replaces the full item sequence, preserving ownership safety.
        void setItems(std::vector<std::unique_ptr<ActionItem>> newItems);
        /// @brief Moves an item within the sequence while preserving order.
        void moveItem(int fromIndex, int toIndex);

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
        std::string id;
        uint32_t m_channel;
    };
}
