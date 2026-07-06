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

Window::~Window() {}

void Window::focus()
{
    // ShowWindow(m_impl->hwnd, SW_SHOW);
}

void Window::hide() {}

void Window::getActiveWindow()
{
    m_impl->hwnd = GetForegroundWindow();
}