/**
 * @file MouseScroll.hpp
 * @brief Mouse wheel / continuous scroll ActionItems.
 */

#pragma once

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/**
 * @brief Simulates mouse wheel with optional modifiers and continuous hold ticks.
 *
 * `dx` / `dy`: Win32 wheel units (120 ≈ one notch). Positive `dy` = up, positive `dx` = right.
 * `modifiers`: same bit mask as AI_Keystroke (Ctrl/Alt/Shift/Win).
 * Tap (`holdDurationSec <= 0`) sends one scroll then releases modifiers.
 * Hold repeats scroll ticks every `intervalSec` for the hold duration (cancel-flush +
 * scheduled cleanup), matching AI_MouseButton.
 */
class AI_MouseScroll : public ActionItem
{
public:
    AI_MouseScroll(int dx = 0,
                   int dy = 120,
                   float holdDurationSec = 0.0f,
                   bool proceed = false,
                   int modifiers = 0,
                   float intervalSec = 0.05f);

    int dx = 0;
    int dy = 120;
    int modifiers = 0;
    float holdDurationSec = 0.0f; ///< Hold time in seconds; <= 0 = single tap.
    float intervalSec = 0.05f;    ///< Delay between ticks while holding.
    bool proceed = false;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

/**
 * @brief Releases modifiers after a held mouse scroll (cleanup / end of hold).
 *
 * Used by AI_MouseScroll cancel-flush and delayed release Actions. Not shown in
 * the settings item picker.
 */
class AI_MouseScrollRelease : public ActionItem
{
public:
    explicit AI_MouseScrollRelease(int modifiers = 0);

    int modifiers = 0;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

/**
 * @brief One scroll pulse without touching modifiers (hold-tick chain only).
 *
 * Built at runtime by AI_MouseScroll; not shown in the settings picker.
 */
class AI_MouseScrollTick : public ActionItem
{
public:
    AI_MouseScrollTick(int dx = 0, int dy = 0);

    int dx = 0;
    int dy = 0;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
