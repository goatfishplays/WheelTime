/**
 * @file MouseButton.hpp
 * @brief Mouse button press / hold ActionItems.
 */

#pragma once

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/**
 * @brief Simulates a mouse button with optional hold behavior.
 *
 * `button`: 0 = left, 1 = right, 2 = middle.
 * Tap (`holdDuration <= 0`) presses then releases. Hold uses press + delayed
 * release (cancel-flush + scheduled cleanup), matching AI_Keystroke.
 */
class AI_MouseButton : public ActionItem
{
public:
    AI_MouseButton(int button = 0, float holdDuration = 0.0f, bool proceed = false);

    int button = 0;
    float holdDuration = 0.0f; ///< Hold time in seconds.
    bool proceed = false;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

/**
 * @brief Releases a previously held mouse button (cleanup / end of hold).
 *
 * Used by AI_MouseButton cancel-flush and delayed release Actions. Not shown in
 * the settings item picker.
 */
class AI_MouseButtonRelease : public ActionItem
{
public:
    explicit AI_MouseButtonRelease(int button = 0);

    int button = 0;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
