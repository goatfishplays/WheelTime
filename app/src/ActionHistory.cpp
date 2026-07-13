/**
 * @file ActionHistory.cpp
 * @brief ActionHistory definitions.
 */

#include "App/ActionHistory.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <algorithm>
#include <utility>

namespace Application
{

ActionHistory::ActionHistory(std::size_t maxRecent)
    : m_maxRecent{maxRecent == 0 ? std::size_t{1} : maxRecent}
{
}

void ActionHistory::recordUse(const std::string &actionId)
{
    if (actionId.empty())
    {
        return;
    }

    std::lock_guard lock{m_mutex};
    m_recent.erase(std::remove(m_recent.begin(), m_recent.end(), actionId), m_recent.end());
    m_recent.insert(m_recent.begin(), actionId);
    if (m_recent.size() > m_maxRecent)
    {
        m_recent.resize(m_maxRecent);
    }
    ++m_counts[actionId];
}

std::string ActionHistory::nthRecentId(int n) const
{
    if (n < 1)
    {
        return {};
    }

    std::lock_guard lock{m_mutex};
    const std::size_t index = static_cast<std::size_t>(n - 1);
    if (index >= m_recent.size())
    {
        return {};
    }
    return m_recent[index];
}

std::string ActionHistory::nthFrequentId(int n) const
{
    if (n < 1)
    {
        return {};
    }

    std::lock_guard lock{m_mutex};
    if (m_counts.empty())
    {
        return {};
    }

    struct Entry
    {
        std::string id;
        std::uint64_t count = 0;
        std::size_t recentRank = 0; // smaller = more recent; large = never in MRU
    };

    std::vector<Entry> entries;
    entries.reserve(m_counts.size());
    for (const auto &[id, count] : m_counts)
    {
        if (count == 0)
        {
            continue;
        }
        Entry entry;
        entry.id = id;
        entry.count = count;
        entry.recentRank = m_recent.size();
        for (std::size_t i = 0; i < m_recent.size(); ++i)
        {
            if (m_recent[i] == id)
            {
                entry.recentRank = i;
                break;
            }
        }
        entries.push_back(std::move(entry));
    }

    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) {
        if (a.count != b.count)
        {
            return a.count > b.count;
        }
        if (a.recentRank != b.recentRank)
        {
            return a.recentRank < b.recentRank;
        }
        return a.id < b.id;
    });

    const std::size_t index = static_cast<std::size_t>(n - 1);
    if (index >= entries.size())
    {
        return {};
    }
    return entries[index].id;
}

void ActionHistory::pruneToLibrary(const std::vector<std::string> &validIds)
{
    std::lock_guard lock{m_mutex};
    const auto isValid = [&validIds](const std::string &id) {
        return std::find(validIds.begin(), validIds.end(), id) != validIds.end();
    };

    m_recent.erase(std::remove_if(m_recent.begin(), m_recent.end(),
                                  [&](const std::string &id) { return !isValid(id); }),
                   m_recent.end());

    for (auto it = m_counts.begin(); it != m_counts.end();)
    {
        if (!isValid(it->first))
        {
            it = m_counts.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool ActionHistory::load(const QString &path)
{
    QFile file(path);
    if (!file.exists())
    {
        return true;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        return false;
    }

    const QJsonObject root = document.object();
    std::vector<std::string> recent;
    for (const QJsonValue &value : root.value("recent").toArray())
    {
        const std::string id = value.toString().toStdString();
        if (id.empty())
        {
            continue;
        }
        if (std::find(recent.begin(), recent.end(), id) == recent.end())
        {
            recent.push_back(id);
        }
    }
    if (recent.size() > m_maxRecent)
    {
        recent.resize(m_maxRecent);
    }

    std::unordered_map<std::string, std::uint64_t> counts;
    const QJsonObject frequent = root.value("frequent").toObject();
    for (auto it = frequent.begin(); it != frequent.end(); ++it)
    {
        const std::string id = it.key().toStdString();
        if (id.empty())
        {
            continue;
        }
        const int count = it.value().toInt(0);
        if (count > 0)
        {
            counts[id] = static_cast<std::uint64_t>(count);
        }
    }

    std::lock_guard lock{m_mutex};
    m_recent = std::move(recent);
    m_counts = std::move(counts);
    return true;
}

bool ActionHistory::save(const QString &path) const
{
    QJsonObject root;
    QJsonArray recentArray;
    QJsonObject frequentObject;

    {
        std::lock_guard lock{m_mutex};
        for (const std::string &id : m_recent)
        {
            recentArray.append(QString::fromStdString(id));
        }
        for (const auto &[id, count] : m_counts)
        {
            frequentObject.insert(QString::fromStdString(id), static_cast<int>(count));
        }
    }

    root.insert("recent", recentArray);
    root.insert("frequent", frequentObject);

    QFileInfo info(path);
    QDir().mkpath(info.absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

std::size_t ActionHistory::recentCount() const
{
    std::lock_guard lock{m_mutex};
    return m_recent.size();
}

std::size_t ActionHistory::frequentEntryCount() const
{
    std::lock_guard lock{m_mutex};
    return m_counts.size();
}

} // namespace Application
