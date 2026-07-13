/**
 * @file MouseButton.hpp
 * @brief Mouse button ActionItem (Platform pending).
 */

#pragma once

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/**
 * @brief Mouse button press, release, or click.
 *
 * `button`: 0 = left, 1 = right, 2 = middle (convention until Platform defines it).
 * `down`: true = press, false = release. For a full click, use press then release
 * (or a future click helper once Platform lands).
 */
class AI_MouseButton : public ActionItem
{
public:
    AI_MouseButton(int button = 0, bool down = true);

    int button = 0;
    bool down = true;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
