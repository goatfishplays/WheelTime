/**
 * @file FuzzyMatch.hpp
 * @brief Small fzf-style fuzzy subsequence scorer for the search palette.
 */

#pragma once

#include <QString>

namespace Application
{

struct FuzzyResult
{
    bool matched{false};
    int score{0};
};

/**
 * @brief Case-insensitive fuzzy subsequence match of @p pattern in @p text.
 *
 * Every pattern character must appear in order inside the text. The score
 * rewards consecutive runs, word-boundary hits, and matches near the start,
 * and mildly penalizes unmatched text so shorter targets rank higher.
 * An empty pattern matches everything with score 0.
 */
[[nodiscard]] FuzzyResult fuzzyMatch(const QString &pattern, const QString &text);

} // namespace Application
