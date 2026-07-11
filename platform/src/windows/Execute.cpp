#include <Platform/Execute.hpp>

#include <windows.h>
#include <iostream>

namespace Platform
{

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
            std::cout << "[Executor] executeKey called\n";

            // TODO: Implement keystroke execution using Windows SendInput.
            // This is where Discord deafen / hotkey simulation would eventually go.
            (void)key;
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