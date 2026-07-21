/**
 * @file Search.cpp
 * @brief AI_Search definitions.
 */

#include "App/ActionItems/Search.hpp"

#include "App/App.hpp"

namespace Application
{

AI_Search::AI_Search(SearchConfig config)
    : config{std::move(config)}
{
}

std::unique_ptr<ActionItem> AI_Search::clone() const
{
    return std::make_unique<AI_Search>(*this);
}

ActionItemKind AI_Search::kind() const
{
    return ActionItemKind::Search;
}

ExecuteResult AI_Search::execute(ActionExecutionContext & /*context*/)
{
    // showSearchOverlay marshals itself to the GUI thread.
    App::instance().showSearchOverlay(config);
    return ExecuteResult::Continue();
}

} // namespace Application
