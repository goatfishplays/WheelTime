/**
 * @file FuzzyMatch.cpp
 * @brief Fuzzy subsequence scorer definitions.
 */

#include "App/FuzzyMatch.hpp"

namespace Application
{

namespace
{

bool isWordBoundary(const QString &text, int index)
{
    if (index == 0)
    {
        return true;
    }
    const QChar prev = text.at(index - 1);
    const QChar curr = text.at(index);
    if (!prev.isLetterOrNumber())
    {
        return true;
    }
    // camelCase boundary
    return prev.isLower() && curr.isUpper();
}

} // namespace

FuzzyResult fuzzyMatch(const QString &pattern, const QString &text)
{
    if (pattern.isEmpty())
    {
        return {true, 0};
    }
    if (text.isEmpty())
    {
        return {false, 0};
    }

    constexpr int kConsecutiveBonus = 12;
    constexpr int kWordBoundaryBonus = 10;
    constexpr int kFirstCharPrefixBonus = 15;
    constexpr int kGapPenalty = 1;
    constexpr int kUnmatchedTrailingPenaltyCap = 10;

    int score = 0;
    int textIndex = 0;
    int prevMatchIndex = -1;

    // Greedy in-order scan: cheap, and good enough ranking for short labels.
    for (int patternIndex = 0; patternIndex < pattern.size(); ++patternIndex)
    {
        const QChar want = pattern.at(patternIndex).toLower();
        bool found = false;
        while (textIndex < text.size())
        {
            if (text.at(textIndex).toLower() == want)
            {
                if (prevMatchIndex >= 0)
                {
                    const int gap = textIndex - prevMatchIndex - 1;
                    if (gap == 0)
                    {
                        score += kConsecutiveBonus;
                    }
                    else
                    {
                        score -= qMin(gap, 20) * kGapPenalty;
                    }
                }
                else if (textIndex == 0)
                {
                    score += kFirstCharPrefixBonus;
                }
                if (isWordBoundary(text, textIndex))
                {
                    score += kWordBoundaryBonus;
                }
                prevMatchIndex = textIndex;
                ++textIndex;
                found = true;
                break;
            }
            ++textIndex;
        }
        if (!found)
        {
            return {false, 0};
        }
    }

    // Prefer tighter targets: penalize leftover unmatched characters a little.
    const int trailing = text.size() - prevMatchIndex - 1;
    score -= qMin(trailing, kUnmatchedTrailingPenaltyCap);

    return {true, score};
}

} // namespace Application
