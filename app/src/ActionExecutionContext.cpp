
#include "App/ActionExecutionContext.hpp"

#include "Action.hpp"
using namespace Application;

ActionExecutionContext::ActionExecutionContext(
    std::unique_ptr<Action> action)
    : m_action(std::move(action))
{
}

Action &ActionExecutionContext::action()
{
    return *m_action;
}

ActionItem *ActionExecutionContext::currentItem()
{
    if (finished())
        return nullptr;

    return m_action->items()[m_currentIndex].get();
}

void ActionExecutionContext::advance()
{
    ++m_currentIndex;
}

bool ActionExecutionContext::finished() const
{
    return m_currentIndex >= m_action->items().size();
}