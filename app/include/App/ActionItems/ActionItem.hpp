/**
 * @file ActionItem.hpp
 * @brief Base ActionItem type and kind enum.
 */

#pragma once

#include <memory>

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
    KeyRelease,
    Socket,
    NthRecent,
    NthFrequent,
    MouseMove,
    MouseButton,
    MouseButtonRelease,
    Cancel,
    Search
};

/// @brief Short console label for @p kind (e.g. "cancel", "hotkey").
[[nodiscard]] const char *actionItemKindName(ActionItemKind kind) noexcept;

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

} // namespace Application
