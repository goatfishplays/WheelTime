/**
 * @file Search.hpp
 * @brief Open-search-palette ActionItem.
 */

#pragma once

#include "App/ActionItems/ActionItem.hpp"
#include "App/SearchConfig.hpp"

namespace Application
{

/// @brief Opens the search palette overlay with a configured filter set.
class AI_Search : public ActionItem
{
public:
    explicit AI_Search(SearchConfig config = {});

    SearchConfig config;

    std::unique_ptr<ActionItem> clone() const override;
    ActionItemKind kind() const override;
    [[nodiscard]] ExecuteResult execute(ActionExecutionContext &context) override;
};

} // namespace Application
