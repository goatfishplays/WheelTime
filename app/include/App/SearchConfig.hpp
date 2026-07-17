/**
 * @file SearchConfig.hpp
 * @brief Filter configuration for the search palette overlay.
 */

#pragma once

#include <string>

namespace Application
{

/**
 * @brief Initial filter set and websearch template for one search palette open.
 *
 * Configured per AI_Search action item; the palette UI can still toggle the
 * four filters at runtime while it is open.
 */
struct SearchConfig
{
    bool searchActions{true};
    bool searchPrograms{true};
    bool searchMenus{true};
    bool webSearch{true};
    /// @brief Websearch URL template; "{query}" is replaced with the
    /// percent-encoded query text.
    std::string webSearchUrl{"https://www.google.com/search?q={query}"};
};

} // namespace Application
