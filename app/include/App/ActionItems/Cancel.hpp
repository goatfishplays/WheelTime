/**
 * @file Cancel.hpp
 * @brief Cancel MostRecent / Channel / All ActionItem.
 */

#pragma once

#include <cstdint>

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/// @brief Which scheduler cancel API an AI_Cancel item invokes.
enum class CancelLevel
{
    /// @brief Cancel the most recently submitted Action on @ref AI_Cancel::channel.
    ///
    /// channel == 0 means the most recent Action on any channel. The Action that
    /// contains this item is never chosen (it would cancel itself).
    MostRecent,
    /// @brief Cancel every cancelable Action on @ref AI_Cancel::channel.
    Channel,
    /// @brief Cancel every cancelable Action.
    All
};

/**
 * @brief Records a cancel request for the scheduler (does not call Scheduler directly).
 *
 * - MostRecent: cancel most recent Action on channel (0 = global); never self.
 * - Channel: cancel every cancelable Action on the configured channel.
 * - All: cancel every cancelable Action.
 */
class AI_Cancel : public ActionItem
{
public:
    explicit AI_Cancel(CancelLevel level = CancelLevel::MostRecent, uint32_t channel = 0);

    CancelLevel level = CancelLevel::MostRecent;
    /**
     * @brief Channel selector.
     *
     * - MostRecent: 0 = most recent across all channels; otherwise that channel only.
     * - Channel: logical channel whose cancelable Actions are cancelled.
     * - All: ignored.
     */
    uint32_t channel = 0;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
