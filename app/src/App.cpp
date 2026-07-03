/**
 * @file App.cpp
 * @author goatfishplays@gmail.com
 * @brief Contains the implementation of the actual app
 * @version 0.1
 * @date 2026-07-02
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "App/App.hpp"
#include <QApplication>
#include <QPushButton>
using namespace Application;

App::App(int &argc, char **argv)
    : activeMenu(nullptr),
      priorMousePos{},
      window{},
      app(argc, argv)
{
    int fake_argc = 1;
    char *fake_argv[] = {(char *)"WheelTimeApp", nullptr};
    QApplication app(fake_argc, fake_argv);

    QPushButton button("Hello world !");
    button.show();

    app.exec();
}