/**
 * @file ProgramIndex.cpp
 * @brief Shortcut index definitions.
 */

#include "App/ProgramIndex.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStandardPaths>

#include <algorithm>

namespace Application
{

ProgramIndex::~ProgramIndex()
{
    joinRefreshThread();
}

void ProgramIndex::joinRefreshThread()
{
    if (m_refreshThread.joinable())
    {
        m_refreshThread.join();
    }
}

QVector<ProgramEntry> ProgramIndex::scanShortcutFolders()
{
    QVector<ProgramEntry> found;
    QHash<QString, bool> seenNames;

    // Candidate shortcut folders. QStandardPaths::ApplicationsLocation can
    // omit the common (all-users) Start Menu depending on Qt version, so the
    // well-known locations are added explicitly from the environment. Start
    // Menu roots are scanned (not just Programs) because some installers
    // drop shortcuts beside Programs instead of inside it.
    QStringList candidates;
    const auto addStartMenuFor = [&candidates](const QString &base)
    {
        if (!base.isEmpty())
        {
            candidates << base + "/Microsoft/Windows/Start Menu";
        }
    };
    addStartMenuFor(qEnvironmentVariable("APPDATA"));
    addStartMenuFor(qEnvironmentVariable("ProgramData"));
    for (const QString &dir :
         QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation))
    {
        QDir parent(dir);
        if (parent.dirName().compare("Programs", Qt::CaseInsensitive) == 0 && parent.cdUp())
        {
            candidates << parent.absolutePath();
        }
        else
        {
            candidates << dir;
        }
    }
    candidates << QStandardPaths::standardLocations(QStandardPaths::DesktopLocation);
    const QString publicDir = qEnvironmentVariable("PUBLIC");
    candidates << (publicDir.isEmpty() ? QStringLiteral("C:/Users/Public/Desktop")
                                       : publicDir + "/Desktop");

    // Normalize and drop duplicates / roots nested inside other roots so the
    // same folder is never scanned twice.
    QStringList roots;
    for (const QString &candidate : candidates)
    {
        const QString normalized = QDir::cleanPath(QDir(candidate).absolutePath());
        if (normalized.isEmpty() || !QDir(normalized).exists())
        {
            continue;
        }
        bool covered = false;
        for (const QString &existing : roots)
        {
            if (normalized.compare(existing, Qt::CaseInsensitive) == 0
                || normalized.startsWith(existing + "/", Qt::CaseInsensitive))
            {
                covered = true;
                break;
            }
        }
        if (!covered)
        {
            roots << normalized;
        }
    }

    for (const QString &root : roots)
    {
        // .lnk covers regular apps; .url covers internet shortcuts, which is
        // how Steam exposes installed games (steam://rungameid/... targets).
        QDirIterator it(root, {"*.lnk", "*.url"}, QDir::Files | QDir::System,
                        QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            const QString path = it.next();
            const QString name = QFileInfo(path).completeBaseName();
            // Launchers conventionally hide uninstallers.
            if (name.contains("uninstall", Qt::CaseInsensitive))
            {
                continue;
            }
            // Earlier roots (user Start Menu) shadow later ones for
            // duplicate shortcut names.
            const QString key = name.toLower();
            if (seenNames.contains(key))
            {
                continue;
            }
            seenNames.insert(key, true);
            found.push_back(ProgramEntry{name, path});
        }
    }

    std::sort(found.begin(), found.end(), [](const ProgramEntry &a, const ProgramEntry &b)
              { return a.name.localeAwareCompare(b.name) < 0; });
    return found;
}

QVector<ProgramEntry> ProgramIndex::entries()
{
    {
        QMutexLocker lock(&m_mutex);
        if (m_everScanned)
        {
            return m_entries;
        }
    }

    // Cache never filled. If the startup preload (refreshAsync) is still
    // running, wait for it rather than duplicating the scan.
    if (m_refreshInProgress.load())
    {
        joinRefreshThread();
    }

    QMutexLocker lock(&m_mutex);
    if (!m_everScanned)
    {
        m_entries = scanShortcutFolders();
        m_everScanned = true;
    }
    return m_entries;
}

void ProgramIndex::refreshAsync()
{
    if (m_refreshInProgress.exchange(true))
    {
        return;
    }
    joinRefreshThread();
    m_refreshThread = std::thread(
        [this]()
        {
            QVector<ProgramEntry> fresh = scanShortcutFolders();
            QMutexLocker relock(&m_mutex);
            m_entries = std::move(fresh);
            m_everScanned = true;
            m_refreshInProgress.store(false);
        });
}

QIcon ProgramIndex::iconFor(const QString &path)
{
    auto it = m_iconCache.find(path);
    if (it != m_iconCache.end())
    {
        return it.value();
    }
    QIcon icon = m_iconProvider.icon(QFileInfo(path));
    m_iconCache.insert(path, icon);
    return icon;
}

QIcon ProgramIndex::cachedIconFor(const QString &path) const
{
    return m_iconCache.value(path);
}

} // namespace Application
