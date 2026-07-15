/**
 * @file Action.hpp
 * @brief Describes a unit of scheduled work: a channel and an ordered item sequence.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <Platform/Inputs.hpp>

#include "App/ActionItems.hpp"

namespace Application
{

/// @brief Display data for one radial-menu / settings slot preview.
struct ActionSlotVisual
{
    std::string label;
    std::string iconPath;

    bool operator==(const ActionSlotVisual &other) const noexcept
    {
        return label == other.label && iconPath == other.iconPath;
    }
};

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

    /// @brief Inserts @p ai at @p index, or appends when out of range.
    void addItem(int index, std::unique_ptr<ActionItem> ai);

    /// @brief Removes the item at @p index when in range.
    void removeItem(int index);

    /// @brief Number of ActionItems in the sequence.
    [[nodiscard]] int itemCount() const;

    /// @brief Stable config ID used by menus and serialization.
    [[nodiscard]] std::string getId() const;
    [[nodiscard]] std::string getName() const;
    /// @brief Optional image shown above the action name on the wheel.
    [[nodiscard]] std::string getIconFilepath() const;
    /// @brief Exposes the item sequence for settings editing/serialization.
    [[nodiscard]] const std::vector<std::unique_ptr<ActionItem>> &getItems() const;
    [[nodiscard]] ActionItem *getItem(int index);
    [[nodiscard]] const ActionItem *getItem(int index) const;
    void setName(const std::string &newName);
    void setId(const std::string &newId);
    void setIconFilepath(const std::string &newIconFilepath);
    /// @brief Replaces the full item sequence, preserving ownership safety.
    void setItems(std::vector<std::unique_ptr<ActionItem>> newItems);
    /// @brief Moves an item within the sequence while preserving order.
    void moveItem(int fromIndex, int toIndex);

    /**
     * @brief Returns the channel this action will run on.
     *
     * 0 = independent (scheduler assigns a private key; does not block peers).
     * >0 = shared FIFO with other Actions on the same channel.
     */
    [[nodiscard]] uint32_t channel() const;
    void setChannel(uint32_t channel) noexcept;

    /**
     * @brief Whether cancelAction/Channel/All may stop this Action.
     *
     * Default true. Cleanup Actions (e.g. delayed KeyUp) should be false so
     * they survive cancelAll and cannot leave hardware in a bad state.
     */
    [[nodiscard]] bool cancelable() const noexcept;
    void setCancelable(bool cancelable) noexcept;

private:
    std::vector<std::unique_ptr<ActionItem>> m_sequence;
    std::string m_name;
    std::string m_iconFilepath;
    std::string m_id;
    uint32_t m_channel = 0;
    bool m_cancelable = true;
};

} // namespace Application
