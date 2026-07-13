/**
 * @file Script.hpp
 * @brief Script / arbitrary path launch ActionItem.
 */

#pragma once

#include <string>

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/// @brief Launches an arbitrary app or script path.
class AI_Script : public ActionItem
{
public:
    explicit AI_Script(std::string filepath = {});

    std::string filepath;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
