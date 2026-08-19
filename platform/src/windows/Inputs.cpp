/**
 * @file Inputs.cpp
 * @brief Windows implementation of input and global hotkeys.
 */

#include "Platform/Inputs.hpp"

#include <windows.h>

#include <algorithm>
#include <iostream>
#include <mutex>
#include <unordered_set>
#include <vector>

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif

using namespace Platform;

namespace
{
bool isVkDownAsync(int vk)
{
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

int packedHotkeyId(int mod, int vk)
{
    return (mod << 16) | (vk & 0xFFFF);
}

bool isModifierVk(int vk)
{
    switch (vk)
    {
    case VK_LCONTROL:
    case VK_RCONTROL:
    case VK_CONTROL:
    case VK_LMENU:
    case VK_RMENU:
    case VK_MENU:
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_SHIFT:
    case VK_LWIN:
    case VK_RWIN:
        return true;
    default:
        return false;
    }
}

/// @brief Clears OS Win-down state after stealing a Win+chord without opening Start.
///
/// Injecting Win keyup alone looks like a lone Win tap (R was swallowed), which
/// opens the Start menu. A brief Ctrl pulse makes the shell treat it as a combo.
void injectWinKeyUps()
{
    INPUT inputs[4] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[0].ki.dwFlags = 0;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_CONTROL;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = VK_LWIN;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_RWIN;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, inputs, sizeof(INPUT));
}
} // namespace

class InputReceiver::Impl
{
public:
    std::mutex mutex;
    std::vector<InputBinding> hookBindings;
    /// @brief Physical (non-injected) keys currently down, tracked by the LL hook.
    std::unordered_set<int> physicalDown;
    std::unordered_set<int> latchedIds;
    std::unordered_set<int> swallowVksUntilUp;
    bool suppressWinKeyUp{false};
    bool captureActive{false};
    HHOOK hook{nullptr};
    HHOOK mouseHook{nullptr};
    HotkeyTriggeredHandler handler{nullptr};
    void *handlerUser{nullptr};
    ChordCaptureHandler captureHandler{nullptr};
    void *captureUser{nullptr};
    OverlayMouseButtonHandler overlayButtonHandler{nullptr};
    void *overlayUser{nullptr};
    bool overlayMouseSession{false};
    bool rawInputRegistered{false};
    POINT lastMousePt{};
    bool lastMousePtValid{false};
    int pendingMouseDx{0};
    int pendingMouseDy{0};
    int pendingMouseWarps{0};

    static Impl *s_active;

    void ensureHookLocked()
    {
        if (hook != nullptr)
        {
            return;
        }
        if (hookBindings.empty() && !captureActive)
        {
            return;
        }
        hook = SetWindowsHookExW(WH_KEYBOARD_LL, &Impl::lowLevelProc, GetModuleHandleW(nullptr), 0);
        if (hook == nullptr)
        {
            std::cerr << "Failed to install WH_KEYBOARD_LL. Error code: " << GetLastError() << "\n";
        }
        else
        {
            // Seed from async state so chords already held mid-install are visible.
            physicalDown.clear();
            seedPhysicalFromAsyncLocked();
            std::cout << "Installed WH_KEYBOARD_LL (bindings=" << hookBindings.size()
                      << ", capture=" << (captureActive ? "yes" : "no") << ")\n";
        }
    }

    void seedPhysicalFromAsyncLocked()
    {
        const int candidates[] = {
            VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU, VK_LSHIFT, VK_RSHIFT, VK_LWIN, VK_RWIN};
        for (int vk : candidates)
        {
            if (isVkDownAsync(vk))
            {
                physicalDown.insert(vk);
            }
        }
    }

    /// @brief Clears hook state under the lock; caller must Unhook outside the lock.
    [[nodiscard]] HHOOK takeHookLocked()
    {
        HHOOK previous = hook;
        hook = nullptr;
        latchedIds.clear();
        swallowVksUntilUp.clear();
        suppressWinKeyUp = false;
        physicalDown.clear();
        return previous;
    }

    void ensureMouseHookLocked()
    {
        if (mouseHook != nullptr || !overlayMouseSession)
        {
            return;
        }
        mouseHook = SetWindowsHookExW(WH_MOUSE_LL, &Impl::lowLevelMouseProc, GetModuleHandleW(nullptr), 0);
        if (mouseHook == nullptr)
        {
            std::cerr << "Failed to install WH_MOUSE_LL. Error code: " << GetLastError() << "\n";
        }
        else
        {
            std::cout << "Installed WH_MOUSE_LL for overlay mouse session\n";
        }
    }

    [[nodiscard]] HHOOK takeMouseHookLocked()
    {
        HHOOK previous = mouseHook;
        mouseHook = nullptr;
        overlayMouseSession = false;
        overlayButtonHandler = nullptr;
        overlayUser = nullptr;
        lastMousePtValid = false;
        pendingMouseDx = 0;
        pendingMouseDy = 0;
        pendingMouseWarps = 0;
        rawInputRegistered = false;
        return previous;
    }

    [[nodiscard]] HHOOK releaseHookIfIdleLocked()
    {
        if (hook != nullptr && hookBindings.empty() && !captureActive)
        {
            return takeHookLocked();
        }
        return nullptr;
    }

    [[nodiscard]] bool isPhysicallyDownLocked(int vk) const
    {
        switch (vk)
        {
        case VK_CONTROL:
            return physicalDown.count(VK_LCONTROL) > 0 || physicalDown.count(VK_RCONTROL) > 0
                   || physicalDown.count(VK_CONTROL) > 0;
        case VK_MENU:
            return physicalDown.count(VK_LMENU) > 0 || physicalDown.count(VK_RMENU) > 0
                   || physicalDown.count(VK_MENU) > 0;
        case VK_SHIFT:
            return physicalDown.count(VK_LSHIFT) > 0 || physicalDown.count(VK_RSHIFT) > 0
                   || physicalDown.count(VK_SHIFT) > 0;
        case VK_LWIN:
        case VK_RWIN:
            return physicalDown.count(vk) > 0;
        default:
            return physicalDown.count(vk) > 0;
        }
    }

    [[nodiscard]] int currentModifierMaskLocked() const
    {
        int mod = 0;
        if (isPhysicallyDownLocked(VK_MENU))
        {
            mod |= 0x0001;
        }
        if (isPhysicallyDownLocked(VK_CONTROL))
        {
            mod |= 0x0002;
        }
        if (isPhysicallyDownLocked(VK_SHIFT))
        {
            mod |= 0x0004;
        }
        if (isPhysicallyDownLocked(VK_LWIN) || isPhysicallyDownLocked(VK_RWIN))
        {
            mod |= 0x0008;
        }
        return mod;
    }

    static LRESULT CALLBACK lowLevelProc(int code, WPARAM wParam, LPARAM lParam)
    {
        Impl *self = s_active;
        if (code == HC_ACTION && self != nullptr)
        {
            if (self->handleLowLevelEvent(wParam, lParam))
            {
                return 1; // Swallow
            }
        }
        return CallNextHookEx(nullptr, code, wParam, lParam);
    }

    void armSwallowLocked(int vk, int mods)
    {
        swallowVksUntilUp.insert(vk);
        if ((mods & 0x0008) != 0)
        {
            // Swallow the physical Win keyup so Start does not open, but we must
            // also inject synthetic Win keyups so the OS does not keep Win stuck.
            suppressWinKeyUp = true;
        }
    }

    [[nodiscard]] bool handleLowLevelEvent(WPARAM wParam, LPARAM lParam)
    {
        const auto *info = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        if (info == nullptr)
        {
            return false;
        }

        const bool injected = (info->flags & LLKHF_INJECTED) != 0;
        const int vk = static_cast<int>(info->vkCode);
        const bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
        const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

        // Injected traffic (including our synthetic Win keyups) must pass through
        // and must not update the physical-down tracker.
        if (injected)
        {
            return false;
        }

        std::unique_lock<std::mutex> lock(mutex);

        if (isDown)
        {
            physicalDown.insert(vk);
        }
        else if (isUp)
        {
            physicalDown.erase(vk);
        }

        if (isUp)
        {
            const bool swallowUp = swallowVksUntilUp.count(vk) > 0;
            swallowVksUntilUp.erase(vk);

            for (auto it = latchedIds.begin(); it != latchedIds.end();)
            {
                if ((*it & 0xFFFF) == (vk & 0xFFFF))
                {
                    it = latchedIds.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            if (suppressWinKeyUp && (vk == VK_LWIN || vk == VK_RWIN))
            {
                // Keep suppressWinKeyUp until both sides are up if both were down;
                // clear when neither Win key remains physically held.
                if (!isPhysicallyDownLocked(VK_LWIN) && !isPhysicallyDownLocked(VK_RWIN))
                {
                    suppressWinKeyUp = false;
                }
                return true;
            }

            return swallowUp;
        }

        if (!isDown || isModifierVk(vk))
        {
            return false;
        }

        // Auto-repeat: key already latched for this press.
        for (int id : latchedIds)
        {
            if ((id & 0xFFFF) == (vk & 0xFFFF))
            {
                return true;
            }
        }

        bool shouldInjectWinUps = false;

        // Settings recorder: capture any chord (including shell-reserved Win+R).
        if (captureActive)
        {
            ChordCaptureHandler capCb = captureHandler;
            void *capUser = captureUser;

            if (vk == VK_ESCAPE)
            {
                captureActive = false;
                captureHandler = nullptr;
                captureUser = nullptr;
                armSwallowLocked(vk, 0);
                lock.unlock();
                if (capCb != nullptr)
                {
                    capCb(0, 0, capUser);
                }
                return true;
            }

            const int mods = currentModifierMaskLocked();
            const int id = packedHotkeyId(mods, vk);
            latchedIds.insert(id);
            armSwallowLocked(vk, mods);
            shouldInjectWinUps = (mods & 0x0008) != 0;
            captureActive = false;
            captureHandler = nullptr;
            captureUser = nullptr;
            lock.unlock();

            if (shouldInjectWinUps)
            {
                injectWinKeyUps();
            }
            if (capCb != nullptr)
            {
                capCb(mods, vk, capUser);
            }
            return true;
        }

        const int mods = currentModifierMaskLocked();
        const InputBinding *matched = nullptr;
        for (const InputBinding &bind : hookBindings)
        {
            if (bind.input == vk && bind.mod == mods)
            {
                matched = &bind;
                break;
            }
        }
        if (matched == nullptr)
        {
            return false;
        }

        const int id = packedHotkeyId(matched->mod, matched->input);
        latchedIds.insert(id);
        armSwallowLocked(vk, matched->mod);
        shouldInjectWinUps = (matched->mod & 0x0008) != 0;

        HotkeyTriggeredHandler cb = handler;
        void *user = handlerUser;
        lock.unlock();

        if (shouldInjectWinUps)
        {
            injectWinKeyUps();
        }
        if (cb != nullptr)
        {
            cb(id, user);
        }
        return true;
    }

    static LRESULT CALLBACK lowLevelMouseProc(int code, WPARAM wParam, LPARAM lParam)
    {
        Impl *self = s_active;
        if (code == HC_ACTION && self != nullptr)
        {
            if (self->handleLowLevelMouse(wParam, lParam))
            {
                return 1;
            }
        }
        return CallNextHookEx(nullptr, code, wParam, lParam);
    }

    [[nodiscard]] bool handleLowLevelMouse(WPARAM wParam, LPARAM lParam)
    {
        const auto *info = reinterpret_cast<MSLLHOOKSTRUCT *>(lParam);
        if (info == nullptr)
        {
            return false;
        }

        OverlayMouseButtonHandler buttonCb = nullptr;
        OverlayMouseButton button = OverlayMouseButton::Left;
        bool pressed = false;
        void *user = nullptr;

        {
            std::lock_guard<std::mutex> lock(mutex);
            if (!overlayMouseSession)
            {
                return false;
            }

            const bool injected = (info->flags & LLMHF_INJECTED) != 0;
            if (lastMousePtValid && wParam == WM_MOUSEMOVE)
            {
                if (injected)
                {
                    ++pendingMouseWarps;
                }
                else if (!rawInputRegistered)
                {
                    // Fallback only: cursor-position deltas rubber-band when a
                    // game recenters without the injected flag (Minecraft).
                    pendingMouseDx += info->pt.x - lastMousePt.x;
                    pendingMouseDy += info->pt.y - lastMousePt.y;
                }
            }
            lastMousePt = info->pt;
            lastMousePtValid = true;

            if (wParam == WM_MOUSEMOVE || injected)
            {
                return false;
            }

            switch (wParam)
            {
            case WM_LBUTTONDOWN:
                button = OverlayMouseButton::Left;
                pressed = true;
                break;
            case WM_LBUTTONUP:
                button = OverlayMouseButton::Left;
                pressed = false;
                break;
            case WM_RBUTTONDOWN:
                button = OverlayMouseButton::Right;
                pressed = true;
                break;
            case WM_RBUTTONUP:
                button = OverlayMouseButton::Right;
                pressed = false;
                break;
            default:
                return false;
            }

            buttonCb = overlayButtonHandler;
            user = overlayUser;
        }

        if (buttonCb != nullptr)
        {
            buttonCb(button, pressed, user);
        }
        // Swallow physical clicks so a captured game does not also receive them.
        return true;
    }
};

InputReceiver::Impl *InputReceiver::Impl::s_active = nullptr;

InputReceiver::InputReceiver()
    : m_impl(std::make_unique<Impl>())
{
    Impl::s_active = m_impl.get();
}

InputReceiver::~InputReceiver()
{
    HHOOK toUnhook = nullptr;
    HHOOK mouseToUnhook = nullptr;
    bool rawRegistered = false;
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        toUnhook = m_impl->takeHookLocked();
        rawRegistered = m_impl->rawInputRegistered;
        mouseToUnhook = m_impl->takeMouseHookLocked();
    }
    if (toUnhook != nullptr)
    {
        UnhookWindowsHookEx(toUnhook);
        std::cout << "Uninstalled WH_KEYBOARD_LL\n";
    }
    if (mouseToUnhook != nullptr)
    {
        UnhookWindowsHookEx(mouseToUnhook);
        std::cout << "Uninstalled WH_MOUSE_LL\n";
    }
    if (rawRegistered)
    {
        RAWINPUTDEVICE rid{};
        rid.usUsagePage = 0x01;
        rid.usUsage = 0x02;
        rid.dwFlags = RIDEV_REMOVE;
        rid.hwndTarget = nullptr;
        RegisterRawInputDevices(&rid, 1, sizeof(rid));
    }
    if (Impl::s_active == m_impl.get())
    {
        Impl::s_active = nullptr;
    }
}

Vec2 InputReceiver::absoluteMousePosition()
{
    POINT p;
    GetCursorPos(&p);
    return Vec2{p.x, p.y};
}

void InputReceiver::setAbsoluteMousePosition(Vec2 position)
{
    SetCursorPos(position.x, position.y);
}

void InputReceiver::releaseCursorClip()
{
    ClipCursor(nullptr);
}

bool InputReceiver::isCursorClipActive() const
{
    RECT clip{};
    if (!GetClipCursor(&clip))
    {
        return false;
    }

    const int virtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int virtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int virtualRight = virtualLeft + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int virtualBottom = virtualTop + GetSystemMetrics(SM_CYVIRTUALSCREEN);
    constexpr int kSlop = 2;
    return clip.left > virtualLeft + kSlop || clip.top > virtualTop + kSlop
           || clip.right < virtualRight - kSlop || clip.bottom < virtualBottom - kSlop;
}

void InputReceiver::beginOverlayMouseSession(void *nativeHwnd, OverlayMouseButtonHandler buttonHandler, void *userData)
{
    POINT cursor{};
    GetCursorPos(&cursor);
    HWND hwnd = static_cast<HWND>(nativeHwnd);

    bool rawOk = false;
    if (hwnd != nullptr)
    {
        RAWINPUTDEVICE rid{};
        rid.usUsagePage = 0x01;
        rid.usUsage = 0x02;
        rid.dwFlags = RIDEV_INPUTSINK;
        rid.hwndTarget = hwnd;
        rawOk = RegisterRawInputDevices(&rid, 1, sizeof(rid)) != FALSE;
        if (!rawOk)
        {
            std::cerr << "Failed to register raw mouse input. Error code: " << GetLastError() << "\n";
        }
        else
        {
            std::cout << "Registered RIDEV_INPUTSINK mouse for overlay session\n";
        }
    }

    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->overlayButtonHandler = buttonHandler;
    m_impl->overlayUser = userData;
    m_impl->overlayMouseSession = true;
    m_impl->rawInputRegistered = rawOk;
    m_impl->lastMousePt = cursor;
    m_impl->lastMousePtValid = true;
    m_impl->pendingMouseDx = 0;
    m_impl->pendingMouseDy = 0;
    m_impl->pendingMouseWarps = 0;
    m_impl->ensureMouseHookLocked();
}

void InputReceiver::endOverlayMouseSession()
{
    HHOOK toUnhook = nullptr;
    bool rawRegistered = false;
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        rawRegistered = m_impl->rawInputRegistered;
        toUnhook = m_impl->takeMouseHookLocked();
    }
    if (toUnhook != nullptr)
    {
        UnhookWindowsHookEx(toUnhook);
        std::cout << "Uninstalled WH_MOUSE_LL\n";
    }
    if (rawRegistered)
    {
        RAWINPUTDEVICE rid{};
        rid.usUsagePage = 0x01;
        rid.usUsage = 0x02;
        rid.dwFlags = RIDEV_REMOVE;
        rid.hwndTarget = nullptr;
        if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
        {
            std::cerr << "Failed to unregister raw mouse input. Error code: " << GetLastError() << "\n";
        }
    }
}

void InputReceiver::pollOverlayMouseDelta(int &dx, int &dy)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    dx = m_impl->pendingMouseDx;
    dy = m_impl->pendingMouseDy;
    m_impl->pendingMouseDx = 0;
    m_impl->pendingMouseDy = 0;
}

int InputReceiver::takeOverlayMouseWarpCount()
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    const int warps = m_impl->pendingMouseWarps;
    m_impl->pendingMouseWarps = 0;
    return warps;
}

bool InputReceiver::processNativeMessage(void *message)
{
    if (message == nullptr)
    {
        return false;
    }

    const MSG *msg = static_cast<MSG *>(message);
    if (msg->message != WM_INPUT)
    {
        return false;
    }

    UINT size = 0;
    GetRawInputData(reinterpret_cast<HRAWINPUT>(msg->lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
    if (size == 0)
    {
        return false;
    }

    std::vector<BYTE> buffer(size);
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(msg->lParam), RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER))
        == static_cast<UINT>(-1))
    {
        return false;
    }

    const auto *raw = reinterpret_cast<RAWINPUT *>(buffer.data());
    if (raw->header.dwType != RIM_TYPEMOUSE)
    {
        return false;
    }

    const RAWMOUSE &mouse = raw->data.mouse;
    if ((mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (!m_impl->overlayMouseSession)
    {
        return false;
    }
    m_impl->pendingMouseDx += mouse.lLastX;
    m_impl->pendingMouseDy += mouse.lLastY;
    return false;
}

void InputReceiver::setHotkeyTriggeredHandler(HotkeyTriggeredHandler handler, void *userData)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->handler = handler;
    m_impl->handlerUser = userData;
}

void InputReceiver::beginChordCapture(ChordCaptureHandler handler, void *userData)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->captureActive = true;
    m_impl->captureHandler = handler;
    m_impl->captureUser = userData;
    m_impl->ensureHookLocked();
}

void InputReceiver::endChordCapture()
{
    HHOOK toUnhook = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        m_impl->captureActive = false;
        m_impl->captureHandler = nullptr;
        m_impl->captureUser = nullptr;
        toUnhook = m_impl->releaseHookIfIdleLocked();
    }
    if (toUnhook != nullptr)
    {
        UnhookWindowsHookEx(toUnhook);
        std::cout << "Uninstalled WH_KEYBOARD_LL\n";
    }
}

void InputReceiver::registerInputBinding(InputBinding bind)
{
    if (bind.useLowLevelHook)
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        const auto already = std::find_if(m_impl->hookBindings.begin(), m_impl->hookBindings.end(),
                                          [&](const InputBinding &existing)
                                          {
                                              return existing.input == bind.input && existing.mod == bind.mod;
                                          });
        if (already == m_impl->hookBindings.end())
        {
            m_impl->hookBindings.push_back(bind);
            std::cout << "Registered LL-hook hotkey: mod=" << bind.mod << ", key=" << bind.input << "\n";
        }
        m_impl->ensureHookLocked();
        return;
    }

    const int id = packedHotkeyId(bind.mod, bind.input);
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

void InputReceiver::unregisterInputBinding(InputBinding bind)
{
    if (bind.useLowLevelHook)
    {
        HHOOK toUnhook = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_impl->mutex);
            m_impl->hookBindings.erase(std::remove_if(m_impl->hookBindings.begin(), m_impl->hookBindings.end(),
                                                      [&](const InputBinding &existing)
                                                      {
                                                          return existing.input == bind.input && existing.mod == bind.mod;
                                                      }),
                                       m_impl->hookBindings.end());
            std::cout << "Unregistered LL-hook hotkey: mod=" << bind.mod << ", key=" << bind.input << "\n";
            if (m_impl->hookBindings.empty())
            {
                toUnhook = m_impl->releaseHookIfIdleLocked();
            }
        }
        if (toUnhook != nullptr)
        {
            UnhookWindowsHookEx(toUnhook);
            std::cout << "Uninstalled WH_KEYBOARD_LL\n";
        }
        return;
    }

    const int id = packedHotkeyId(bind.mod, bind.input);
    if (!UnregisterHotKey(NULL, id))
    {
        std::cerr << "Failed to unregister hotkey: id=" << id << ". Error code: " << GetLastError() << "\n";
    }
    else
    {
        std::cout << "Successfully unregistered hotkey: id=" << id << "\n";
    }
}

bool InputReceiver::isHotkeyMessage(void *message, int &hotkeyIdOut)
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

bool InputReceiver::isVirtualKeyDown(int vk) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (m_impl->hook != nullptr)
    {
        return m_impl->isPhysicallyDownLocked(vk);
    }
    return isVkDownAsync(vk);
}

bool InputReceiver::isChordHeld(int mod, int vk) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    const auto down = [this](int key) -> bool
    {
        if (m_impl->hook != nullptr)
        {
            return m_impl->isPhysicallyDownLocked(key);
        }
        return isVkDownAsync(key);
    };

    if (!down(vk))
    {
        return false;
    }
    if ((mod & 0x0001) != 0 && !down(VK_MENU))
    {
        return false;
    }
    if ((mod & 0x0002) != 0 && !down(VK_CONTROL))
    {
        return false;
    }
    if ((mod & 0x0004) != 0 && !down(VK_SHIFT))
    {
        return false;
    }
    if ((mod & 0x0008) != 0 && !down(VK_LWIN) && !down(VK_RWIN))
    {
        return false;
    }
    return true;
}

bool InputReceiver::isChordFullyReleased(int mod, int vk) const
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    const auto down = [this](int key) -> bool
    {
        if (m_impl->hook != nullptr)
        {
            return m_impl->isPhysicallyDownLocked(key);
        }
        return isVkDownAsync(key);
    };

    if (down(vk))
    {
        return false;
    }
    if ((mod & 0x0001) != 0 && down(VK_MENU))
    {
        return false;
    }
    if ((mod & 0x0002) != 0 && down(VK_CONTROL))
    {
        return false;
    }
    if ((mod & 0x0004) != 0 && down(VK_SHIFT))
    {
        return false;
    }
    if ((mod & 0x0008) != 0 && (down(VK_LWIN) || down(VK_RWIN)))
    {
        return false;
    }
    return true;
}
