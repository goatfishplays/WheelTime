//
// #include "lib.h"
// #include <iostream>

// int main()
// {
//     // std::cout << "Hello, World\n";
//     printWorld();
//     return 0;
// }

#define UNICODE
#include <windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd,
                            UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"MyWindow";

    WNDCLASS wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Hello World",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    ShowWindow(hwnd, nCmdShow);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}