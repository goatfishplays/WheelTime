/**
 * @file MouseMove.hpp
 * @brief Absolute mouse move ActionItem.
 */

#pragma once

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/// @brief Moves the mouse cursor to an absolute screen position.
class AI_MouseMove : public ActionItem
{
public:
    AI_MouseMove(int x = 0, int y = 0);

    int x = 0;
    int y = 0;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
