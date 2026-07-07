/**
 * @file Execute.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2026-07-05
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Platform/Execute.hpp"
#include "Platform/Inputs.hpp"
#include <windows.h>
#include <iostream>
using namespace Platform;

class Executor::Impl
{
};

Executor::Executor() {}
Executor::~Executor() {}

void Executor::setMousePos(int x, int y)
{
    if (SetCursorPos(x, y))
    {
        std::cout << "Set cursor to (" << x << ',' << y << ")\n";
    }
    else
    {
        std::cerr << "Failed to set cursor pos\n";
    }
}
void Executor::executeKey(InputBind key) {}
void Executor::executeScript(std::string filepath) {}