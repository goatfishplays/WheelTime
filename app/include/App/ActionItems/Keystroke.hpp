/**
 * @file Keystroke.hpp
 * @brief Keystroke press / hold ActionItems.
 */

#pragma once

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/**
 * @brief Simulates a keystroke with optional modifiers and hold behavior.
 *
 * Hold uses keyDown + delayed keyUp (cancel-flush + scheduled cleanup).
 */
class AI_Keystroke : public ActionItem
{
public:
    AI_Keystroke(int keycode = 0, int modifiers = 0, float holdDurationSec = 0.0f, bool proceed = false);

    int keycode = 0;
    int modifiers = 0;
    float holdDurationSec = 0.0f; ///< Hold time in seconds.
    bool proceed = false;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

/**
 * @brief Releases a previously held key (cleanup / end of hold).
 *
 * Used by AI_Keystroke cancel-flush and delayed release Actions. Not shown in
 * the settings item picker.
 */
class AI_KeyRelease : public ActionItem
{
public:
    AI_KeyRelease(int keycode = 0, int modifiers = 0);

    int keycode = 0;
    int modifiers = 0;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
