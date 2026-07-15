/**
 * @file main.cpp
 * @brief WheelTime launcher entry point.
 */

// #include "lib.h"
#include <iostream>
#include <QApplication>
#include <QPushButton>
#include <App/App.hpp>
#include <QFile>
#include <QDebug>
#include <QDir>

int main(int argc, char *argv[])
{
    std::cout << "Hello, World\n";

    int fake_argc = 1;
    char *fake_argv[] = {(char *)"WheelTimeApp", nullptr};
    // Application::App app = Application::App(fake_argc, fake_argv);
    // app.exitMenu();

    QApplication qtApp(fake_argc, fake_argv);
    // Launcher shell is a Qt::Tool overlay and is ignored for "last window" checks.
    // Closing Settings (a real Qt::Window) must not quit the hotkey-resident app.
    qtApp.setQuitOnLastWindowClosed(false);

    // QPushButton button("Hello world !");

    // button.setText("My text");
    // button.setToolTip("A tooltip");
    // button.show();

    Application::App &app = Application::App::getInstance();
    // QFile file("resources/styles/defaultWheel.qss");
    QFile file(":/styles/defaultWheel.qss");

    qDebug() << QDir(":/").entryList(QDir::AllEntries);
    qDebug() << QDir(":/styles").entryList(QDir::AllEntries);
    qDebug() << QFile::exists(":/styles/defaultWheel.qss");

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
    return 0;
}