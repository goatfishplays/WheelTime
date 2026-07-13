/**
 * @file NthFrequent.hpp
 * @brief nth-frequent history ActionItem.
 */

#pragma once

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/// @brief Schedules a copy of the nth most frequently wheel-launched Action (1-based).
class AI_nthFrequent : public ActionItem
{
public:
    explicit AI_nthFrequent(int n = 1);

    int n = 1; ///< 1 = most frequent.

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
