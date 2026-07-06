/**
 * @file Inputs.cpp
 * @author
 * @brief WindowsOS gathering of inputs
 * @version 0.1
 * @date 2026-06-29
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Platform/Inputs.hpp"
#include <windows.h>
#include <iostream>
using namespace Platform;

class InputRcvr::Impl
{
};

InputRcvr::InputRcvr() {}
InputRcvr::~InputRcvr() {}

Vec2 InputRcvr::getAbsoluteMousePosition()
{
    POINT mousePos;
    if (GetCursorPos(&mousePos))
    {
        return Vec2{(int)mousePos.x, (int)mousePos.y};
    }
    else
    {
        std::cerr << "Failed to get cursor position\n";
        // TODO: remind me how to do errors and raise one where
        return Vec2{-1, -1};
    }
}

// Vec2 InputRcvr::getRelativeMousePosition()
// {

// }

void InputRcvr::registerInputBinding(InputBind inputBind) {}
void InputRcvr::unregisterInputBinding(InputBind inputBind) {}