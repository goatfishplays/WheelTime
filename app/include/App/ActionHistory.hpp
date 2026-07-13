/**
 * @file ActionHistory.hpp
 * @brief Persistent MRU + frequency tracking for library Actions.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <QString>

namespace Application
{

/**
 * @brief Tracks which library Actions the user has launched from the wheel.
 *
 * - Recent: unique MRU list (index 0 = most recent).
 * - Frequent: launch counts keyed by library action id.
 *
 * Lookups are 1-based (n=1 is most recent / most frequent). Thread-safe.
 */
class ActionHistory
{
public:
    explicit ActionHistory(std::size_t maxRecent = 64);

    /// @brief Records a user launch of @p actionId (ignored if empty).
    void recordUse(const std::string &actionId);

    /**
     * @brief Library action id at 1-based recent rank @p n.
     * @return Empty string if @p n is out of range or no history.
     */
    [[nodiscard]] std::string nthRecentId(int n) const;

    /**
     * @brief Library action id at 1-based frequency rank @p n.
     *
     * Ties break toward the more recently used id.
     * @return Empty string if @p n is out of range or no history.
     */
    [[nodiscard]] std::string nthFrequentId(int n) const;

    /// @brief Drops ids that are not in @p validIds (e.g. after config reload).
    void pruneToLibrary(const std::vector<std::string> &validIds);

    /// @brief Loads recent/frequent state from JSON at @p path (missing file is ok).
    bool load(const QString &path);

    /// @brief Writes recent/frequent state to JSON at @p path.
    bool save(const QString &path) const;

    [[nodiscard]] std::size_t recentCount() const;
    [[nodiscard]] std::size_t frequentEntryCount() const;

private:
    mutable std::mutex m_mutex;
    std::vector<std::string> m_recent;
    std::unordered_map<std::string, std::uint64_t> m_counts;
    std::size_t m_maxRecent;
};

} // namespace Application
