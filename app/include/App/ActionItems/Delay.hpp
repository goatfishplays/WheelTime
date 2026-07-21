/**
 * @file Delay.hpp
 * @brief Delay ActionItem.
 */

#pragma once

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/// @brief Delays the current Action before the next item runs.
class AI_Delay : public ActionItem
{
public:
    explicit AI_Delay(int durationMs = 0);

    int durationMs = 0; ///< Delay in milliseconds.

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
