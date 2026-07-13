/**
 * @file Close.hpp
 * @brief Close-launcher ActionItem.
 */

#pragma once

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/// @brief Closes the currently visible launcher UI.
class AI_Close : public ActionItem
{
public:
    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
