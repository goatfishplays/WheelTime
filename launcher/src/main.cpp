/**
 * @file main.cpp
 * @brief WheelTime launcher entry point.
 */

#include <QApplication>
#include <QCoreApplication>
#include <App/App.hpp>
#include <QFile>
#include <QDebug>

int main(int argc, char *argv[])
{
    int fake_argc = 1;
    char *fake_argv[] = {(char *)"WheelTimeApp", nullptr};

    QApplication qtApp(fake_argc, fake_argv);
    QCoreApplication::setOrganizationName("WheelTime");
    QCoreApplication::setApplicationName("WheelTime");
    // Launcher shell is a Qt::Tool overlay and is ignored for "last window" checks.
    // Closing Settings (a real Qt::Window) must not quit the hotkey-resident app.
    qtApp.setQuitOnLastWindowClosed(false);

    Application::App &app = Application::App::getInstance();
    QFile file(":/styles/defaultWheel.qss");

    if (file.open(QFile::ReadOnly))
    {
        qApp->setStyleSheet(file.readAll());
    }
    else
    {
        qDebug() << "Failed to open resource";
    }
    // QApplication::setStyle("Fusion");

    return qtApp.exec();
}
