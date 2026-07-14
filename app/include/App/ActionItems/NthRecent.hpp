/**
 * @file NthRecent.hpp
 * @brief nth-recent history ActionItem.
 */

#pragma once

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/// @brief Schedules a copy of the nth most recently wheel-launched Action (1-based).
class AI_nthRecent : public ActionItem
{
public:
    explicit AI_nthRecent(int n = 1);

    int n = 1; ///< 1 = most recent.

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
