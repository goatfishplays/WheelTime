/**
 * @file ActionItems.hpp
 * @brief Building blocks that compose an Action.
 */

#pragma once

#include <memory>
#include <string>

#include "App/ExecuteResult.hpp"

namespace Application
{

class ActionExecutionContext;

/**
 * @brief Runtime/editor discriminator for supported action item types.
 */
enum class ActionItemKind
{
    Base,
    LaunchApp,
    Script,
    Delay,
    Menu,
    Close,
    Keystroke,
    Socket,
    NthRecent,
    NthFrequent
};

/**
 * @brief One executable step inside an Action.
 *
 * ActionItems never talk to the scheduler directly. They perform work and/or
 * record requests on ActionExecutionContext, then return an ExecuteResult.
 */
class ActionItem
{
public:
    ActionItem() = default;
    virtual ~ActionItem() = default;

    virtual std::unique_ptr<ActionItem> clone() const;
    virtual ActionItemKind kind() const;

    [[nodiscard]] virtual ExecuteResult execute(ActionExecutionContext &context);
};

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

/**
 * @brief Simulates a hotkey with optional modifiers and hold behavior.
 *
 * When `proceed` is false and `holdDuration` is positive, the current Action
 * delays before the next item (workers never sleep).
 */
class AI_Keystroke : public ActionItem
{
public:
    AI_Keystroke(int keycode = 0, int modifiers = 0, float holdDuration = 0.0f, bool proceed = false);

    int keycode = 0;
    int modifiers = 0;
    float holdDuration = 0.0f; ///< Hold time in seconds.
    bool proceed = false;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

/// @brief Delays the current Action before the next item runs.
class AI_Delay : public ActionItem
{
public:
    explicit AI_Delay(int durationMs = 0);

    int duration = 0; ///< Delay in milliseconds.

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

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

/// @brief Closes the currently visible launcher UI.
class AI_Close : public ActionItem
{
public:
    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

/// @brief Sends a socket message (platform support pending).
class AI_Socket : public ActionItem
{
public:
    std::string socketMsg;
    std::string outputDst;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

/// @brief Executes the nth most recent unique action (history support pending).
class AI_nthRecent : public ActionItem
{
public:
    int n = 0;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

/// @brief Executes the nth most frequently used action (history support pending).
class AI_nthFrequent : public ActionItem
{
public:
    int n = 0;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
