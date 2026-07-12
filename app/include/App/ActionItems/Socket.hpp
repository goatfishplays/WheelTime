/**
 * @file Socket.hpp
 * @brief Socket send ActionItem (Platform pending).
 */

#pragma once

#include <string>

#include "App/ActionItems/ActionItem.hpp"

namespace Application
{

/// @brief Sends a socket message (platform support pending).
class AI_Socket : public ActionItem
{
public:
    std::string socketMsg;
    std::string outputDst;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
