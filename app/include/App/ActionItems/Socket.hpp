/**
 * @file Socket.hpp
 * @brief Multi-protocol network send ActionItem.
 */

#pragma once

#include <string>

#include <Platform/Execute.hpp>

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/// @brief One-shot network send (UDP, TCP, HTTP, or WebSocket).
class AI_Socket : public ActionItem
{
public:
    AI_Socket(Platform::SocketProtocol protocol = Platform::SocketProtocol::Udp,
              std::string destination = {},
              std::string message = {},
              std::string httpMethod = "POST");

    Platform::SocketProtocol protocol = Platform::SocketProtocol::Udp;
    std::string outputDst;   ///< host:port or URL
    std::string socketMsg;   ///< payload / body
    std::string httpMethod = "POST"; ///< used only for HTTP

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
