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

void Window::focus()
{
    ShowWindow(m_impl->hwnd, SW_SHOW);
}