/**
 * @file LogWindow.cpp
 * @brief Optional on-screen viewer for the runtime log tee.
 */

#include "App/LogWindow.hpp"

#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QUrl>
#include <QVBoxLayout>

using namespace Application;

LogWindow::LogWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("WheelTime Log"));
    setWindowFlags(Qt::Window);
    resize(780, 480);
    setAttribute(Qt::WA_DeleteOnClose, false);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    m_view = new QPlainTextEdit(this);
    m_view->setReadOnly(true);
    m_view->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_view->setMaximumBlockCount(5000);
    QFont mono = m_view->font();
    mono.setFamily(QStringLiteral("Consolas"));
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(10);
    m_view->setFont(mono);
    root->addWidget(m_view, 1);

    auto *bottom = new QHBoxLayout();
    m_pathLabel = new QLabel(this);
    m_pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_pathLabel->setWordWrap(true);
    bottom->addWidget(m_pathLabel, 1);

    auto *openFolder = new QPushButton(QStringLiteral("Open Folder"), this);
    connect(openFolder, &QPushButton::clicked, this, [this]()
            {
                const QString path = m_pathLabel->property("logPath").toString();
                if (path.isEmpty())
                {
                    return;
                }
                QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
            });
    bottom->addWidget(openFolder);
    root->addLayout(bottom);
}

void LogWindow::setLogFilePath(const QString &path)
{
    m_pathLabel->setProperty("logPath", path);
    m_pathLabel->setText(QStringLiteral("Log file: %1").arg(path));
}

void LogWindow::seed(const QStringList &lines)
{
    m_view->setPlainText(lines.join(QChar('\n')));
    m_view->moveCursor(QTextCursor::End);
}

void LogWindow::appendLine(const QString &text)
{
    m_view->appendPlainText(text);
}

void LogWindow::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}
