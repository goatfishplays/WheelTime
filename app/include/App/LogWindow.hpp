/**
 * @file LogWindow.hpp
 * @brief Optional on-screen viewer for the runtime log tee.
 */

#pragma once

#include <QWidget>

class QLabel;
class QPlainTextEdit;

namespace Application
{
/**
 * @brief Simple scrollable log viewer opened from the tray menu.
 *
 * Closing hides the window; the Log singleton keeps ownership so reopen is cheap.
 */
class LogWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LogWindow(QWidget *parent = nullptr);

    void setLogFilePath(const QString &path);
    void seed(const QStringList &lines);

public slots:
    void appendLine(const QString &text);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QPlainTextEdit *m_view{nullptr};
    QLabel *m_pathLabel{nullptr};
};
} // namespace Application
