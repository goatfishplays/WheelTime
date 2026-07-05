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

#include "App/Gui.hpp"
using namespace Application;

Gui::Gui(QWidget *parent) : QWidget(parent)
{
    setFixedSize(600, 400);
    label = new QLabel("Hello, World", this);
    label->show();

    // this->show();
}