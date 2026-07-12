/**
 * @file NthRecent.hpp
 * @brief nth-recent history ActionItem (App history pending).
 */

#pragma once

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/// @brief Executes the nth most recent unique action (history support pending).
class AI_nthRecent : public ActionItem
{
public:
    int n = 0;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
