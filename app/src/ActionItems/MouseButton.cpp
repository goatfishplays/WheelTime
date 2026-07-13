/**
 * @file MouseButton.cpp
 * @brief AI_MouseButton definitions.
 */

#include "App/ActionItems/MouseButton.hpp"

#include <iostream>

namespace Application
{

AI_MouseButton::AI_MouseButton(int button, bool down)
    : button{button}
    , down{down}
{
}

std::unique_ptr<ActionItem> AI_MouseButton::clone() const
{
    return std::make_unique<AI_MouseButton>(*this);
}

ActionItemKind AI_MouseButton::kind() const
{
    return ActionItemKind::MouseButton;
}

ExecuteResult AI_MouseButton::execute(ActionExecutionContext & /*context*/)
{
    // When Platform exposes mouse button press/release:
    // App::getInstance().executor.mouseButton(button, down);
    // // or: mouseDown(button) / mouseUp(button)
    std::cerr << "[AI_MouseButton] would "
              << (down ? "press" : "release") << " button=" << button << '\n';
    return ExecuteResult::Continue();
}

} // namespace Application
