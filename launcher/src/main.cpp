/**
 * @file main.cpp
 * @brief WheelTime launcher entry point.
 */

#include <QApplication>
#include <QCoreApplication>
#include <QIODevice>
#include <QLocalServer>
#include <QLocalSocket>
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

    const QString singleInstanceServerName = "WheelTime.SingleInstance";
    QLocalSocket existingInstance;
    existingInstance.connectToServer(singleInstanceServerName, QIODevice::WriteOnly);
    if (existingInstance.waitForConnected(150))
    {
        // WheelTime is normally background-resident. A second exe launch should
        // act like "bring up settings" instead of creating a second hotkey owner.
        existingInstance.write("show-settings\n");
        existingInstance.flush();
        existingInstance.waitForBytesWritten(150);
        return 0;
    }

    QLocalServer::removeServer(singleInstanceServerName);
    QLocalServer singleInstanceServer;
    if (!singleInstanceServer.listen(singleInstanceServerName))
    {
        qWarning() << "WheelTime is already running or the single-instance server could not start:"
                   << singleInstanceServer.errorString();
        return 0;
    }

    Application::App &app = Application::App::instance();
    QObject::connect(&singleInstanceServer, &QLocalServer::newConnection, [&singleInstanceServer, &app]()
                     {
                         const auto handleCommand = [&app](QLocalSocket *client)
                         {
                             const QByteArray command = client->readAll();
                             if (command.contains("show-settings"))
                             {
                                 app.showSettingsWindow();
                             }
                             client->disconnectFromServer();
                         };

                         while (QLocalSocket *client = singleInstanceServer.nextPendingConnection())
                         {
                             QObject::connect(client, &QLocalSocket::readyRead, client, [client, handleCommand]()
                                              { handleCommand(client); });
                             QObject::connect(client, &QLocalSocket::disconnected, client, &QLocalSocket::deleteLater);
                             if (client->bytesAvailable() > 0)
                             {
                                 handleCommand(client);
                             }
                         }
                     });
    // QApplication::setStyle("Fusion");

    return qtApp.exec();
}
