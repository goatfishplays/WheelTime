/**
 * @file Socket.cpp
 * @brief AI_Socket definitions.
 */

#include "App/ActionItems/Socket.hpp"

#include <iostream>

namespace Application
{

std::unique_ptr<ActionItem> AI_Socket::clone() const
{
    return std::make_unique<AI_Socket>(*this);
}

ActionItemKind AI_Socket::kind() const
{
    return ActionItemKind::Socket;
}

ExecuteResult AI_Socket::execute(ActionExecutionContext & /*context*/)
{
    // When Platform gains a socket/send API:
    // App::getInstance().executor.sendSocket(outputDst, socketMsg);
    std::cerr << "[AI_Socket] would send msg='" << socketMsg << "' dst='" << outputDst << "'\n";
    return ExecuteResult::Continue();
}

} // namespace Application
