/**
 * @file MouseMove.cpp
 * @brief AI_MouseMove definitions.
 */

#include "App/ActionItems/MouseMove.hpp"

#include "App/App.hpp"

#include <iostream>

namespace Application
{

AI_MouseMove::AI_MouseMove(int x, int y)
    : x{x}
    , y{y}
{
}

std::unique_ptr<ActionItem> AI_MouseMove::clone() const
{
    return std::make_unique<AI_MouseMove>(*this);
}

ActionItemKind AI_MouseMove::kind() const
{
    return ActionItemKind::MouseMove;
}

ExecuteResult AI_MouseMove::execute(ActionExecutionContext & /*context*/)
{
    std::cerr << "[AI_MouseMove] setMousePos x=" << x << " y=" << y << '\n';
    App::instance().executor().setMousePos(x, y);
    return ExecuteResult::Continue();
}

} // namespace Application
