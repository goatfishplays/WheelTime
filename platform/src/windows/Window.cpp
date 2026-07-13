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

Window::Window(void *nativeHandle)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->hwnd = static_cast<HWND>(nativeHandle);
}

Window::~Window() {}

namespace
{
    void refreshWindowFrame(HWND hwnd)
    {
        SetWindowPos(
            hwnd,
            NULL,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
}

void Window::focus()
{
    if (!m_impl || !m_impl->hwnd)
        return;

    HWND hwnd = m_impl->hwnd;

    // Restore if minimized, otherwise just show
    if (IsIconic(hwnd))
    {
        ShowWindow(hwnd, SW_RESTORE);
    }
    else
    {
        ShowWindow(hwnd, SW_SHOW);
    }

    // Attach thread input of current foreground window to our thread
    // to bypass focus restrictions and bring the window to front.
    HWND foreWindow = GetForegroundWindow();
    if (foreWindow != hwnd)
    {
        DWORD dwForeThread = GetWindowThreadProcessId(foreWindow, NULL);
        DWORD dwCurrentThread = GetCurrentThreadId();

        if (dwForeThread != dwCurrentThread)
        {
            AttachThreadInput(dwForeThread, dwCurrentThread, TRUE);
            SetForegroundWindow(hwnd);
            SetFocus(hwnd);
            AttachThreadInput(dwForeThread, dwCurrentThread, FALSE);
        }
        else
        {
            SetForegroundWindow(hwnd);
            SetFocus(hwnd);
        }
    }

    SetActiveWindow(hwnd);
}

void Window::getActiveWindow()
{
    m_impl->hwnd = GetForegroundWindow();
}

void Window::hide()
{
    if (m_impl && m_impl->hwnd)
    {
        ShowWindow(m_impl->hwnd, SW_HIDE);
    }
}

void Window::showNoActivate()
{
    if (!m_impl || !m_impl->hwnd)
    {
        return;
    }

    ShowWindow(m_impl->hwnd, SW_SHOWNOACTIVATE);
    SetWindowPos(
        m_impl->hwnd,
        HWND_TOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void Window::setTopmost(bool enabled)
{
    if (!m_impl || !m_impl->hwnd)
    {
        return;
    }

    SetWindowPos(
        m_impl->hwnd,
        enabled ? HWND_TOPMOST : HWND_NOTOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void Window::setNonActivating(bool enabled)
{
    if (!m_impl || !m_impl->hwnd)
    {
        return;
    }

    LONG_PTR exStyle = GetWindowLongPtr(m_impl->hwnd, GWL_EXSTYLE);
    if (enabled)
    {
        exStyle |= WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
    }
    else
    {
        exStyle &= ~WS_EX_NOACTIVATE;
    }
    SetWindowLongPtr(m_impl->hwnd, GWL_EXSTYLE, exStyle);
    refreshWindowFrame(m_impl->hwnd);
}

void Window::setTransparentOverlay(bool enabled)
{
    if (!m_impl || !m_impl->hwnd)
    {
        return;
    }

    // Qt already uses a layered window internally when the top-level widget
    // has WA_TranslucentBackground enabled. We only make sure the window keeps
    // the transparency-capable style bit; we intentionally avoid calling
    // SetLayeredWindowAttributes here because that conflicts with Qt's own
    // UpdateLayeredWindowIndirect rendering path for translucent widgets.
    LONG_PTR exStyle = GetWindowLongPtr(m_impl->hwnd, GWL_EXSTYLE);
    if (enabled)
    {
        exStyle |= WS_EX_LAYERED;
    }
    else
    {
        exStyle &= ~WS_EX_LAYERED;
    }
    SetWindowLongPtr(m_impl->hwnd, GWL_EXSTYLE, exStyle);

    refreshWindowFrame(m_impl->hwnd);
}

void Window::setClickThrough(bool enabled)
{
    if (!m_impl || !m_impl->hwnd)
    {
        return;
    }

    LONG_PTR exStyle = GetWindowLongPtr(m_impl->hwnd, GWL_EXSTYLE);
    if (enabled)
    {
        exStyle |= WS_EX_TRANSPARENT;
    }
    else
    {
        exStyle &= ~WS_EX_TRANSPARENT;
    }
    SetWindowLongPtr(m_impl->hwnd, GWL_EXSTYLE, exStyle);
    refreshWindowFrame(m_impl->hwnd);
}

void Window::setBounds(const WindowRect &bounds)
{
    if (!m_impl || !m_impl->hwnd)
    {
        return;
    }

    SetWindowPos(
        m_impl->hwnd,
        NULL,
        bounds.x,
        bounds.y,
        bounds.width,
        bounds.height,
        SWP_NOACTIVATE | SWP_NOZORDER);
}

WindowRect Window::monitorBoundsForPoint(int x, int y) const
{
    POINT point{x, y};
    HMONITOR monitor = MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    GetMonitorInfo(monitor, &info);

    return WindowRect{
        info.rcMonitor.left,
        info.rcMonitor.top,
        info.rcMonitor.right - info.rcMonitor.left,
        info.rcMonitor.bottom - info.rcMonitor.top};
}
