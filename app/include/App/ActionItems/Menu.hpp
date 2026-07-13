/**
 * @file Menu.hpp
 * @brief Switch-to-menu ActionItem.
 */

#pragma once

#include <string>

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/// @brief Switches the launcher to another menu by stable menu ID.
class AI_Menu : public ActionItem
{
public:
    explicit AI_Menu(std::string menuId = {});

    std::string menuId;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
