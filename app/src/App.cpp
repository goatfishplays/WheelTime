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
#include <Platform/Window.hpp>
#include <Platform/Inputs.hpp>
#include <Platform/Execute.hpp>
#include <QApplication>
#include <QPushButton>
using namespace Application;

// TODO: Mbe just pass QApp in
App::App(int &argc, char **argv)
    : activeMenu(nullptr),
      priorMousePos{},
      priorWindow{},
      gui(0)
{
    // int fake_argc = 1;
    // char *fake_argv[] = {(char *)"WheelTimeApp", nullptr};
    // QApplication app(fake_argc, fake_argv);

    // QPushButton button("Hello world !");

    // button.setText("My text");
    // button.setToolTip("A tooltip");
    // button.show();

    // app.exec();
    gui.show();
}

App::~App() {}

Menu *App::getActiveMenu() { return activeMenu; }

void App::setActiveMenu(Menu &menu)
{
}

void App::gatherPriors()
{
    priorMousePos = inputRcvr.getAbsoluteMousePosition();
    priorWindow.getActiveWindow();
}

void App::restorePriors()
{
}

void App::exitMenu()
{
    executor.setMousePos(priorMousePos.x, priorMousePos.y);
    priorWindow.focus();
}

void App::runAction(Action &action) {}