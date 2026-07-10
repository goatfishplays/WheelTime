#include <Platform/Execute.hpp>

#include <windows.h>
#include <iostream>
#include <vector>

namespace Platform
{
    namespace
    {
        void pushKeyboardInput(std::vector<INPUT> &inputs, WORD vk, DWORD flags = 0)
        {
            INPUT input{};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = vk;
            input.ki.dwFlags = flags;
            inputs.push_back(input);
        }
    }

    class Executor::Impl
    {
    public:
        void executeScript(const std::string &filepath)
        {
            std::cout << "[Executor] Launching script/app: " << filepath << "\n";

            ShellExecuteA(
                NULL,
                "open",
                filepath.c_str(),
                NULL,
                NULL,
                SW_SHOWNORMAL);
        }

        void executeKey(InputBind key)
        {
            std::vector<INPUT> inputs;
            inputs.reserve(10);

            if (key.mod & 0x0008) // MOD_WIN
                pushKeyboardInput(inputs, VK_LWIN);
            if (key.mod & 0x0002) // MOD_CONTROL
                pushKeyboardInput(inputs, VK_CONTROL);
            if (key.mod & 0x0001) // MOD_ALT
                pushKeyboardInput(inputs, VK_MENU);
            if (key.mod & 0x0004) // MOD_SHIFT
                pushKeyboardInput(inputs, VK_SHIFT);

            pushKeyboardInput(inputs, static_cast<WORD>(key.input));
            pushKeyboardInput(inputs, static_cast<WORD>(key.input), KEYEVENTF_KEYUP);

            if (key.mod & 0x0004)
                pushKeyboardInput(inputs, VK_SHIFT, KEYEVENTF_KEYUP);
            if (key.mod & 0x0001)
                pushKeyboardInput(inputs, VK_MENU, KEYEVENTF_KEYUP);
            if (key.mod & 0x0002)
                pushKeyboardInput(inputs, VK_CONTROL, KEYEVENTF_KEYUP);
            if (key.mod & 0x0008)
                pushKeyboardInput(inputs, VK_LWIN, KEYEVENTF_KEYUP);

            const UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
            if (sent != inputs.size())
            {
                std::cerr << "Failed to send full key input sequence. Error code: " << GetLastError() << "\n";
            }
        }
    };

    Executor::Executor()
        : m_impl(std::make_unique<Impl>())
    {
    }

    Executor::~Executor() = default;

    void Executor::executeScript(std::string filepath)
    {
        m_impl->executeScript(filepath);
    }

    void Executor::executeKey(InputBind key)
    {
        m_impl->executeKey(key);
    }

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
}
