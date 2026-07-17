/**
 * @file ProgramIndex.hpp
 * @brief Start Menu shortcut index for the search palette's program filter.
 */

#pragma once

#include <QFileIconProvider>
#include <QHash>
#include <QIcon>
#include <QMutex>
#include <QString>
#include <QVector>

#include <atomic>
#include <thread>

namespace Application
{

struct ProgramEntry
{
    /// @brief Display name (shortcut basename without .lnk).
    QString name;
    /// @brief Absolute path to the .lnk file; launched via ShellExecute "open".
    QString path;
};

/**
 * @brief Indexes .lnk shortcuts from the user and common Start Menus plus
 * the user and public desktops.
 *
 * refreshAsync() scans on a background thread; call it at app startup to
 * preload the index and once per palette open to pick up newly installed
 * programs. entries() serves the cache, waiting for an in-flight first scan
 * (or scanning synchronously) if the cache was never filled.
 */
class ProgramIndex
{
public:
    ~ProgramIndex();

    /// @brief Cached entries; blocks on the first scan if none has completed.
    QVector<ProgramEntry> entries();

    /// @brief Rescans shortcut folders on a background thread (no-op while
    /// a refresh is already running).
    void refreshAsync();

    /// @brief Icon for a shortcut path, extracting on first use. GUI thread only.
    QIcon iconFor(const QString &path);

    /// @brief Already-extracted icon for a path, or a null icon. Never blocks
    /// on icon extraction, so it is safe in per-keystroke paths.
    [[nodiscard]] QIcon cachedIconFor(const QString &path) const;

private:
    static QVector<ProgramEntry> scanShortcutFolders();
    void joinRefreshThread();

    mutable QMutex m_mutex;
    QVector<ProgramEntry> m_entries;
    bool m_everScanned{false};
    std::atomic<bool> m_refreshInProgress{false};
    std::thread m_refreshThread;

    QFileIconProvider m_iconProvider;
    QHash<QString, QIcon> m_iconCache;
};

} // namespace Application
