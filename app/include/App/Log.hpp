/**
 * @file Log.hpp
 * @brief Runtime log tee: file + optional on-screen window for cout/cerr/Qt messages.
 */

#pragma once

#include <QObject>
#include <QString>

namespace Application
{
class LogWindow;

/**
 * @brief Captures std::cout / std::cerr and Qt messages into a log file and UI.
 *
 * Call install() once after QApplication + org/app names are set. Existing
 * `std::cerr` call sites keep working without changes.
 */
class Log : public QObject
{
    Q_OBJECT

public:
    static Log &instance();

    /// @brief Default path under AppConfigLocation (wheeltime.log).
    [[nodiscard]] static QString defaultLogPath();

    /**
     * @brief Redirect cout/cerr and install a Qt message handler.
     * @param logFilePath Destination file; parent dirs are created as needed.
     */
    void install(const QString &logFilePath = defaultLogPath());
    /// @brief Restore prior stream buffers and Qt message handler.
    void uninstall();

    [[nodiscard]] QString logFilePath() const;
    [[nodiscard]] bool isInstalled() const noexcept;

    /// @brief Shows (or reuses) the optional log viewer window.
    void showWindow();

    /// @brief Append a line from any thread (queued to the UI when needed).
    void append(const QString &text);

signals:
    void lineAppended(const QString &text);

private:
    Log();
    ~Log() override;

    class Impl;
    Impl *m_impl;
};

} // namespace Application
