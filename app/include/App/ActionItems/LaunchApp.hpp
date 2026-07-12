/**
 * @file LaunchApp.hpp
 * @brief Preset / custom app launch ActionItem.
 */

#pragma once

#include <string>

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/**
 * @brief Launches a curated preset or a custom app target.
 *
 * `presetId == "custom"` means launch `customTarget` instead.
 */
class AI_LaunchApp : public ActionItem
{
public:
    explicit AI_LaunchApp(std::string presetId = "custom", std::string customTarget = {});

    std::string presetId;
    std::string customTarget;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
