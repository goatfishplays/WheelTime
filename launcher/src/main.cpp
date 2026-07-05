/**
 * @file main.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-07-04
 *
 * @copyright Copyright (c) 2026
 *
 */

// #include "lib.h"
#include <iostream>
#include <QApplication>
#include <QPushButton>
#include <App/App.hpp>

int main(int argc, char *argv[])
{
    std::cout << "Hello, World\n";

    int fake_argc = 1;
    char *fake_argv[] = {(char *)"WheelTimeApp", nullptr};
    // Application::App app = Application::App(fake_argc, fake_argv);
    // app.exitMenu();

    QApplication qtApp(fake_argc, fake_argv);

    // QPushButton button("Hello world !");

    // button.setText("My text");
    // button.setToolTip("A tooltip");
    // button.show();

    Application::App app = Application::App(argc, argv);

    return qtApp.exec();
    return 0;
}