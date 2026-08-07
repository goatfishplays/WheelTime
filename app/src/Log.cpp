/**
 * @file Log.cpp
 * @brief Runtime log tee: file + optional on-screen window for cout/cerr/Qt messages.
 */

#include "App/Log.hpp"
#include "App/LogWindow.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QtGlobal>

#include <functional>
#include <fstream>
#include <iostream>
#include <memory>
#include <streambuf>
#include <string>

using namespace Application;

namespace
{
constexpr int kMaxRingLines = 5000;

class TeeStreambuf final : public std::streambuf
{
public:
    TeeStreambuf(std::streambuf *original, std::function<void(const std::string &)> onLine)
        : m_original(original), m_onLine(std::move(onLine))
    {
    }

protected:
    int overflow(int ch) override
    {
        if (ch == traits_type::eof())
        {
            return sync() == 0 ? !traits_type::eof() : traits_type::eof();
        }

        const char c = static_cast<char>(ch);
        m_pending.push_back(c);
        if (m_original != nullptr)
        {
            m_original->sputc(c);
        }
        if (c == '\n')
        {
            flushPending();
        }
        return ch;
    }

    int sync() override
    {
        if (m_original != nullptr)
        {
            m_original->pubsync();
        }
        if (!m_pending.empty())
        {
            flushPending();
        }
        return 0;
    }

private:
    void flushPending()
    {
        std::string line = std::move(m_pending);
        m_pending.clear();
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
        {
            line.pop_back();
        }
        if (m_onLine)
        {
            m_onLine(line);
        }
    }

    std::streambuf *m_original{nullptr};
    std::function<void(const std::string &)> m_onLine;
    std::string m_pending;
};

void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    const char *level = "INFO";
    switch (type)
    {
    case QtDebugMsg:
        level = "DEBUG";
        break;
    case QtInfoMsg:
        level = "INFO";
        break;
    case QtWarningMsg:
        level = "WARN";
        break;
    case QtCriticalMsg:
        level = "ERROR";
        break;
    case QtFatalMsg:
        level = "FATAL";
        break;
    }
    Log::instance().append(QStringLiteral("[%1] %2").arg(QLatin1String(level), msg));
}
} // namespace

class Log::Impl
{
public:
    QMutex mutex;
    QString path;
    std::ofstream file;
    QStringList ring;
    bool installed{false};
    std::streambuf *oldCout{nullptr};
    std::streambuf *oldCerr{nullptr};
    std::unique_ptr<TeeStreambuf> coutBuf;
    std::unique_ptr<TeeStreambuf> cerrBuf;
    QtMessageHandler oldQtHandler{nullptr};
    LogWindow *window{nullptr};
};

Log::Log()
    : m_impl(new Impl)
{
}

Log::~Log()
{
    uninstall();
    delete m_impl;
    m_impl = nullptr;
}

Log &Log::instance()
{
    static Log log;
    return log;
}

QString Log::defaultLogPath()
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (root.isEmpty())
    {
        root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    if (root.isEmpty())
    {
        root = QDir::home().filePath(QStringLiteral(".wheeltime"));
    }
    return QDir(root).filePath(QStringLiteral("wheeltime.log"));
}

QString Log::logFilePath() const
{
    return m_impl->path;
}

bool Log::isInstalled() const noexcept
{
    return m_impl->installed;
}

void Log::install(const QString &logFilePath)
{
    if (m_impl->installed)
    {
        return;
    }

    const QFileInfo info(logFilePath);
    QDir().mkpath(info.absolutePath());

    {
        QMutexLocker lock(&m_impl->mutex);
        m_impl->path = info.absoluteFilePath();
        m_impl->file.open(m_impl->path.toStdString(), std::ios::out | std::ios::app);
    }

    auto emitLine = [this](const std::string &line)
    {
        append(QString::fromStdString(line));
    };

    m_impl->oldCout = std::cout.rdbuf();
    m_impl->oldCerr = std::cerr.rdbuf();
    m_impl->coutBuf = std::make_unique<TeeStreambuf>(m_impl->oldCout, emitLine);
    m_impl->cerrBuf = std::make_unique<TeeStreambuf>(m_impl->oldCerr, emitLine);
    std::cout.rdbuf(m_impl->coutBuf.get());
    std::cerr.rdbuf(m_impl->cerrBuf.get());

    m_impl->oldQtHandler = qInstallMessageHandler(qtMessageHandler);
    m_impl->installed = true;

    append(QStringLiteral("=== WheelTime log started %1 ===")
               .arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
    append(QStringLiteral("Log file: %1").arg(m_impl->path));
}

void Log::uninstall()
{
    if (!m_impl->installed)
    {
        return;
    }

    append(QStringLiteral("=== WheelTime log stopped %1 ===")
               .arg(QDateTime::currentDateTime().toString(Qt::ISODate)));

    std::cout.flush();
    std::cerr.flush();

    if (m_impl->coutBuf)
    {
        std::cout.rdbuf(m_impl->oldCout);
        m_impl->coutBuf.reset();
    }
    if (m_impl->cerrBuf)
    {
        std::cerr.rdbuf(m_impl->oldCerr);
        m_impl->cerrBuf.reset();
    }
    qInstallMessageHandler(m_impl->oldQtHandler);
    m_impl->oldQtHandler = nullptr;

    {
        QMutexLocker lock(&m_impl->mutex);
        if (m_impl->file.is_open())
        {
            m_impl->file.flush();
            m_impl->file.close();
        }
    }

    if (m_impl->window != nullptr)
    {
        delete m_impl->window;
        m_impl->window = nullptr;
    }

    m_impl->installed = false;
}

void Log::append(const QString &text)
{
    const QString line = text.trimmed();
    if (line.isEmpty())
    {
        return;
    }

    {
        QMutexLocker lock(&m_impl->mutex);
        if (m_impl->file.is_open())
        {
            m_impl->file << line.toStdString() << '\n';
            m_impl->file.flush();
        }
        m_impl->ring.push_back(line);
        while (m_impl->ring.size() > kMaxRingLines)
        {
            m_impl->ring.removeFirst();
        }
    }

    // Always queue to the GUI thread so worker-pool cerr is safe.
    QMetaObject::invokeMethod(
        this,
        [this, line]()
        {
            emit lineAppended(line);
        },
        Qt::QueuedConnection);
}

void Log::showWindow()
{
    if (m_impl->window == nullptr)
    {
        m_impl->window = new LogWindow();
        m_impl->window->setLogFilePath(m_impl->path);
        connect(this, &Log::lineAppended, m_impl->window, &LogWindow::appendLine, Qt::QueuedConnection);

        QStringList snapshot;
        {
            QMutexLocker lock(&m_impl->mutex);
            snapshot = m_impl->ring;
        }
        m_impl->window->seed(snapshot);
    }

    m_impl->window->show();
    m_impl->window->raise();
    m_impl->window->activateWindow();
}
