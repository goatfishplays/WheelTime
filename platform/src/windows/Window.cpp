/**
 * @file Window.cpp
 * @author
 * @brief WindowsOS implementation of window controls
 * @version 0.1
 * @date 2026-06-29
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "Platform/Window.hpp"
#include <windows.h>
using namespace Platform;

class Window::Impl
{
public:
    HWND hwnd{};
    HINSTANCE instance{};
};

Window::Window()
    : m_impl(std::make_unique<Impl>())
{
}

Window::Window(void* nativeHandle)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->hwnd = static_cast<HWND>(nativeHandle);
}

Window::~Window() {}

void Window::getActiveWindow()
{
    if (m_impl) {
        m_impl->hwnd = GetForegroundWindow();
    }
}

void Window::focus()
{
    if (!m_impl || !m_impl->hwnd) return;

    HWND hwnd = m_impl->hwnd;

    // Restore if minimized, otherwise just show
    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    } else {
        ShowWindow(hwnd, SW_SHOW);
    }

    // Attach thread input of current foreground window to our thread 
    // to bypass focus restrictions and bring the window to front.
    HWND foreWindow = GetForegroundWindow();
    if (foreWindow != hwnd) {
        DWORD dwForeThread = GetWindowThreadProcessId(foreWindow, NULL);
        DWORD dwCurrentThread = GetCurrentThreadId();

        if (dwForeThread != dwCurrentThread) {
            AttachThreadInput(dwForeThread, dwCurrentThread, TRUE);
            SetForegroundWindow(hwnd);
            SetFocus(hwnd);
            AttachThreadInput(dwForeThread, dwCurrentThread, FALSE);
        } else {
            SetForegroundWindow(hwnd);
            SetFocus(hwnd);
        }
    }
    
    SetActiveWindow(hwnd);
}

void Window::hide()
{
    if (m_impl && m_impl->hwnd) {
        ShowWindow(m_impl->hwnd, SW_HIDE);
    }
}