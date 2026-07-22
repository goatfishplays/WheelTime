/**
 * @file Socket.cpp
 * @brief AI_Socket definitions.
 */

#include "App/ActionItems/Socket.hpp"

#include "App/App.hpp"

#include <iostream>

namespace Application
{
namespace
{

const char *socketProtocolName(Platform::SocketProtocol protocol) noexcept
{
    switch (protocol)
    {
    case Platform::SocketProtocol::Udp:
        return "udp";
    case Platform::SocketProtocol::Tcp:
        return "tcp";
    case Platform::SocketProtocol::Http:
        return "http";
    case Platform::SocketProtocol::WebSocket:
        return "websocket";
    }
    return "unknown";
}

} // namespace

AI_Socket::AI_Socket(Platform::SocketProtocol protocol,
                     std::string destination,
                     std::string message,
                     std::string httpMethod)
    : protocol{protocol}
    , outputDst{std::move(destination)}
    , socketMsg{std::move(message)}
    , httpMethod{std::move(httpMethod)}
{
}

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
    Platform::SocketSendRequest request;
    request.protocol = protocol;
    request.destination = outputDst;
    request.message = socketMsg;
    request.httpMethod = httpMethod.empty() ? "POST" : httpMethod;

    std::cerr << "[AI_Socket] send protocol=" << socketProtocolName(protocol)
              << " dst='" << outputDst << "'";
    if (protocol == Platform::SocketProtocol::Http)
    {
        std::cerr << " method=" << request.httpMethod;
    }
    std::cerr << " msg='" << socketMsg << "'\n";

    if (!App::instance().executor().sendSocket(request))
    {
        std::cerr << "[AI_Socket] send failed protocol=" << socketProtocolName(protocol)
                  << " dst='" << outputDst << "'\n";
    }
    else
    {
        std::cerr << "[AI_Socket] send ok\n";
    }
    return ExecuteResult::Continue();
}

} // namespace Application
