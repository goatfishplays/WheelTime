/**
 * @file Menu.cpp
 * @brief AI_Menu definitions.
 */

#include "App/ActionItems/Menu.hpp"

#include "App/App.hpp"

#include <iostream>

namespace Application
{

AI_Menu::AI_Menu(std::string menuId)
    : menuId{std::move(menuId)}
{
}

std::unique_ptr<ActionItem> AI_Menu::clone() const
{
    return std::make_unique<AI_Menu>(*this);
}

ActionItemKind AI_Menu::kind() const
{
    return ActionItemKind::Menu;
}

ExecuteResult AI_Menu::execute(ActionExecutionContext & /*context*/)
{
    App &app = App::getInstance();
    Menu *targetMenu = app.findMenuById(menuId);
    if (targetMenu != nullptr)
    {
        std::cerr << "[AI_Menu] show menuId='" << menuId << "'\n";
        app.showGui(targetMenu);
    }
    else
    {
        std::cerr << "[AI_Menu] failed to find menuId='" << menuId << "'\n";
    }
    return ExecuteResult::Continue();
}

} // namespace Application
