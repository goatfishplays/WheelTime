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
App::App()
    : activeMenu(nullptr),
      gui(0),
      priorMousePos{},
      priorWindow{},
      inputRcvr(),
      executor()
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
    executor.setMousePos(priorMousePos.x, priorMousePos.y);
    priorWindow.focus();
}

void App::exitMenu()
{
    restorePriors();
}

void App::runAction(Action &action) {}