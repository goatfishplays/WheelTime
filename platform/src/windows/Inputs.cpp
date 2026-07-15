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

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif

using namespace Platform;

namespace
{
bool isVkDown(int vk)
{
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}
} // namespace

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

void InputRcvr::setAbsoluteMousePosition(Vec2 position)
{
    SetCursorPos(position.x, position.y);
}

// Vec2 InputRcvr::getRelativeMousePosition()
// {
//     // Default fallback to absolute if no window focus context is provided
//     return getAbsoluteMousePosition();
// }

void InputRcvr::registerInputBinding(InputBind bind)
{
    // Pack id from the stored user mods only — never bake MOD_NOREPEAT into the id.
    const int id = (bind.mod << 16) | (bind.input & 0xFFFF);
    // MOD_NOREPEAT: one WM_HOTKEY per physical press (no hold/auto-repeat flash).
    const UINT fsModifiers = static_cast<UINT>(bind.mod) | MOD_NOREPEAT;
    if (!RegisterHotKey(NULL, id, fsModifiers, bind.input))
    {
        std::cerr << "Failed to register hotkey: mod=" << bind.mod << ", key=" << bind.input
                  << ". Error code: " << GetLastError() << "\n";
    }
    else
    {
        std::cout << "Successfully registered hotkey: mod=" << bind.mod << ", key=" << bind.input << "\n";
    }
}

void InputRcvr::unregisterInputBinding(InputBind bind)
{
    int id = (bind.mod << 16) | (bind.input & 0xFFFF);
    if (!UnregisterHotKey(NULL, id))
    {
        std::cerr << "Failed to unregister hotkey: id=" << id << ". Error code: " << GetLastError() << "\n";
    }
    else
    {
        std::cout << "Successfully unregistered hotkey: id=" << id << "\n";
    }
}

bool InputRcvr::isHotkeyMessage(void *message, int &hotkeyIdOut)
{
    if (!message)
        return false;
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_HOTKEY)
    {
        hotkeyIdOut = static_cast<int>(msg->wParam);
        return true;
    }
    return false;
}

bool InputRcvr::isVirtualKeyDown(int vk) const
{
    return isVkDown(vk);
}

bool InputRcvr::isChordHeld(int mod, int vk) const
{
    if (!isVkDown(vk))
    {
        return false;
    }

    // RegisterHotKey mod flags (winuser.h): ALT=1, CTRL=2, SHIFT=4, WIN=8.
    if ((mod & 0x0001) != 0 && !isVkDown(VK_MENU))
    {
        return false;
    }
    if ((mod & 0x0002) != 0 && !isVkDown(VK_CONTROL))
    {
        return false;
    }
    if ((mod & 0x0004) != 0 && !isVkDown(VK_SHIFT))
    {
        return false;
    }
    if ((mod & 0x0008) != 0 && !isVkDown(VK_LWIN) && !isVkDown(VK_RWIN))
    {
        return false;
    }

    return true;
}
