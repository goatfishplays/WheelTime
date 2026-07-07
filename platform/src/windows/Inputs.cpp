/**
 * @file Inputs.cpp
 * @author 
 * @brief Windows implementation of input and global hotkeys
 * @version 0.1
 * @date 2026-07-05
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
public:
};

InputRcvr::InputRcvr()
    : m_impl(std::make_unique<Impl>())
{
}

InputRcvr::~InputRcvr()
{
}

Vec2 InputRcvr::getAbsoluteMousePosition()
{
    POINT p;
    GetCursorPos(&p);
    return Vec2{p.x, p.y};
}

Vec2 InputRcvr::getRelativeMousePosition()
{
    // Default fallback to absolute if no window focus context is provided
    return getAbsoluteMousePosition();
}

void InputRcvr::registerInputBinding(InputBind bind)
{
    int id = (bind.mod << 16) | (bind.input & 0xFFFF);
    if (!RegisterHotKey(NULL, id, bind.mod, bind.input)) {
        std::cerr << "Failed to register hotkey: mod=" << bind.mod << ", key=" << bind.input 
                  << ". Error code: " << GetLastError() << "\n";
    } else {
        std::cout << "Successfully registered hotkey: mod=" << bind.mod << ", key=" << bind.input << "\n";
    }
}

void InputRcvr::unregisterInputBinding(InputBind bind)
{
    int id = (bind.mod << 16) | (bind.input & 0xFFFF);
    if (!UnregisterHotKey(NULL, id)) {
        std::cerr << "Failed to unregister hotkey: id=" << id << ". Error code: " << GetLastError() << "\n";
    } else {
        std::cout << "Successfully unregistered hotkey: id=" << id << "\n";
    }
}

bool InputRcvr::isHotkeyMessage(void* message, int& hotkeyIdOut)
{
    if (!message) return false;
    MSG* msg = static_cast<MSG*>(message);
    if (msg->message == WM_HOTKEY) {
        hotkeyIdOut = static_cast<int>(msg->wParam);
        return true;
    }
    return false;
}
