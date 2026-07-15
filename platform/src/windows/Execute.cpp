#include "Platform/Execute.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
#include <winhttp.h>

#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

namespace Platform
{
namespace
{
constexpr DWORD kNetworkTimeoutMs = 5000;

bool isMediaVirtualKey(WORD vk)
{
    switch (vk)
    {
    case VK_VOLUME_MUTE:
    case VK_VOLUME_DOWN:
    case VK_VOLUME_UP:
    case VK_MEDIA_NEXT_TRACK:
    case VK_MEDIA_PREV_TRACK:
    case VK_MEDIA_STOP:
    case VK_MEDIA_PLAY_PAUSE:
    case VK_LAUNCH_MEDIA_SELECT:
        return true;
    default:
        return false;
    }
}

bool isExtendedVirtualKey(WORD vk)
{
    switch (vk)
    {
    case VK_LEFT:
    case VK_UP:
    case VK_RIGHT:
    case VK_DOWN:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_END:
    case VK_HOME:
    case VK_INSERT:
    case VK_DELETE:
    case VK_DIVIDE:
    case VK_NUMLOCK:
    case VK_RCONTROL:
    case VK_RMENU:
    case VK_LWIN:
    case VK_RWIN:
    case VK_APPS:
        return true;
    default:
        return isMediaVirtualKey(vk);
    }
}

void pushKeyboardInput(std::vector<INPUT> &inputs, WORD vk, DWORD flags = 0)
{
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;

    // Media keys don't map to usable scancodes via MapVirtualKey; SendInput must
    // use the virtual-key path. Normal keys keep the scancode path so games /
    // apps that ignore VK still see the physical key.
    if (isMediaVirtualKey(vk))
    {
        input.ki.wScan = 0;
        input.ki.dwFlags = flags | KEYEVENTF_EXTENDEDKEY;
    }
    else
    {
        input.ki.wScan = static_cast<WORD>(MapVirtualKey(vk, MAPVK_VK_TO_VSC));
        input.ki.dwFlags = flags | KEYEVENTF_SCANCODE;
        if (isExtendedVirtualKey(vk))
        {
            input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        }
    }

    inputs.push_back(input);
}

void sendInputs(std::vector<INPUT> &inputs)
{
    if (inputs.empty())
    {
        return;
    }
    const UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    if (sent != inputs.size())
    {
        std::cerr << "[Executor] Failed to send full input sequence. Error code: "
                  << GetLastError() << "\n";
    }
}

void appendModifierDowns(std::vector<INPUT> &inputs, int mod)
{
    if (mod & 0x0008) // MOD_WIN
        pushKeyboardInput(inputs, VK_LWIN);
    if (mod & 0x0002) // MOD_CONTROL
        pushKeyboardInput(inputs, VK_LCONTROL);
    if (mod & 0x0001) // MOD_ALT
        pushKeyboardInput(inputs, VK_LMENU);
    if (mod & 0x0004) // MOD_SHIFT
        pushKeyboardInput(inputs, VK_LSHIFT);
}

void appendModifierUps(std::vector<INPUT> &inputs, int mod)
{
    // Release both left/right so a physical RCtrl/RShift leftover is cleared.
    if (mod & 0x0004)
    {
        pushKeyboardInput(inputs, VK_LSHIFT, KEYEVENTF_KEYUP);
        pushKeyboardInput(inputs, VK_RSHIFT, KEYEVENTF_KEYUP);
    }
    if (mod & 0x0001)
    {
        pushKeyboardInput(inputs, VK_LMENU, KEYEVENTF_KEYUP);
        pushKeyboardInput(inputs, VK_RMENU, KEYEVENTF_KEYUP);
    }
    if (mod & 0x0002)
    {
        pushKeyboardInput(inputs, VK_LCONTROL, KEYEVENTF_KEYUP);
        pushKeyboardInput(inputs, VK_RCONTROL, KEYEVENTF_KEYUP);
    }
    if (mod & 0x0008)
    {
        pushKeyboardInput(inputs, VK_LWIN, KEYEVENTF_KEYUP);
        pushKeyboardInput(inputs, VK_RWIN, KEYEVENTF_KEYUP);
    }
}

struct HostPort
{
    std::string host;
    std::string port;
};

std::optional<HostPort> parseHostPort(std::string_view destination)
{
    if (destination.empty())
    {
        return std::nullopt;
    }

    // [ipv6]:port
    if (destination.front() == '[')
    {
        const auto close = destination.find(']');
        if (close == std::string_view::npos || close + 1 >= destination.size()
            || destination[close + 1] != ':')
        {
            return std::nullopt;
        }
        HostPort out;
        out.host = std::string(destination.substr(1, close - 1));
        out.port = std::string(destination.substr(close + 2));
        if (out.host.empty() || out.port.empty())
        {
            return std::nullopt;
        }
        return out;
    }

    const auto colon = destination.rfind(':');
    if (colon == std::string_view::npos || colon == 0 || colon + 1 >= destination.size())
    {
        return std::nullopt;
    }
    HostPort out;
    out.host = std::string(destination.substr(0, colon));
    out.port = std::string(destination.substr(colon + 1));
    return out;
}

struct ParsedUrl
{
    bool secure = false;
    std::wstring host;
    INTERNET_PORT port = 0;
    std::wstring path; // includes query
};

std::optional<ParsedUrl> parseUrl(std::string_view url, bool websocket)
{
    std::string_view rest = url;
    bool secure = false;

    auto startsWithIgnoreCase = [](std::string_view text, std::string_view prefix) -> bool {
        if (text.size() < prefix.size())
        {
            return false;
        }
        for (size_t i = 0; i < prefix.size(); ++i)
        {
            const auto a = static_cast<unsigned char>(text[i]);
            const auto b = static_cast<unsigned char>(prefix[i]);
            if (std::tolower(a) != std::tolower(b))
            {
                return false;
            }
        }
        return true;
    };

    auto consumeScheme = [&](std::string_view scheme, bool isSecure) -> bool {
        if (!startsWithIgnoreCase(rest, scheme))
        {
            return false;
        }
        rest.remove_prefix(scheme.size());
        secure = isSecure;
        return true;
    };

    if (websocket)
    {
        if (!consumeScheme("wss://", true) && !consumeScheme("ws://", false))
        {
            return std::nullopt;
        }
    }
    else
    {
        if (!consumeScheme("https://", true) && !consumeScheme("http://", false))
        {
            return std::nullopt;
        }
    }

    if (rest.empty())
    {
        return std::nullopt;
    }

    const auto slash = rest.find('/');
    const std::string_view authority =
        slash == std::string_view::npos ? rest : rest.substr(0, slash);
    const std::string_view path =
        slash == std::string_view::npos ? "/" : rest.substr(slash);

    if (authority.empty())
    {
        return std::nullopt;
    }

    std::string_view hostView = authority;
    INTERNET_PORT port = secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

    auto parsePort = [](std::string_view text) -> std::optional<INTERNET_PORT> {
        try
        {
            const int value = std::stoi(std::string(text));
            if (value <= 0 || value > 65535)
            {
                return std::nullopt;
            }
            return static_cast<INTERNET_PORT>(value);
        }
        catch (...)
        {
            return std::nullopt;
        }
    };

    if (authority.front() == '[')
    {
        const auto close = authority.find(']');
        if (close == std::string_view::npos)
        {
            return std::nullopt;
        }
        hostView = authority.substr(1, close - 1);
        if (close + 1 < authority.size() && authority[close + 1] == ':')
        {
            const auto parsedPort = parsePort(authority.substr(close + 2));
            if (!parsedPort)
            {
                return std::nullopt;
            }
            port = *parsedPort;
        }
    }
    else
    {
        const auto colon = authority.rfind(':');
        if (colon != std::string_view::npos)
        {
            hostView = authority.substr(0, colon);
            const auto parsedPort = parsePort(authority.substr(colon + 1));
            if (!parsedPort)
            {
                return std::nullopt;
            }
            port = *parsedPort;
        }
    }

    if (hostView.empty())
    {
        return std::nullopt;
    }

    ParsedUrl out;
    out.secure = secure;
    out.port = port;
    out.host.assign(hostView.begin(), hostView.end());
    out.path.assign(path.begin(), path.end());
    return out;
}

class WinsockGuard
{
public:
    WinsockGuard()
    {
        WSADATA data{};
        m_ok = (WSAStartup(MAKEWORD(2, 2), &data) == 0);
        if (!m_ok)
        {
            std::cerr << "[Executor] WSAStartup failed: " << WSAGetLastError() << "\n";
        }
    }

    ~WinsockGuard()
    {
        if (m_ok)
        {
            WSACleanup();
        }
    }

    explicit operator bool() const noexcept { return m_ok; }

private:
    bool m_ok = false;
};

bool sendViaSocket(int sockType, const HostPort &hp, const std::string &message)
{
    WinsockGuard winsock;
    if (!winsock)
    {
        return false;
    }

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = sockType;
    hints.ai_protocol = sockType == SOCK_DGRAM ? IPPROTO_UDP : IPPROTO_TCP;

    addrinfo *result = nullptr;
    const int status = getaddrinfo(hp.host.c_str(), hp.port.c_str(), &hints, &result);
    if (status != 0 || result == nullptr)
    {
        std::cerr << "[Executor] getaddrinfo failed for " << hp.host << ':' << hp.port
                  << " err=" << status << "\n";
        return false;
    }

    bool ok = false;
    for (addrinfo *rp = result; rp != nullptr; rp = rp->ai_next)
    {
        SOCKET sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == INVALID_SOCKET)
        {
            continue;
        }

        DWORD timeout = kNetworkTimeoutMs;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeout),
                   sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char *>(&timeout),
                   sizeof(timeout));

        if (sockType == SOCK_DGRAM)
        {
            const int sent = sendto(
                sock,
                message.data(),
                static_cast<int>(message.size()),
                0,
                rp->ai_addr,
                static_cast<int>(rp->ai_addrlen));
            ok = sent == static_cast<int>(message.size());
            if (!ok)
            {
                std::cerr << "[Executor] UDP sendto failed: " << WSAGetLastError() << "\n";
            }
            closesocket(sock);
            if (ok)
            {
                break;
            }
            continue;
        }

        // TCP
        if (connect(sock, rp->ai_addr, static_cast<int>(rp->ai_addrlen)) != 0)
        {
            std::cerr << "[Executor] TCP connect failed: " << WSAGetLastError() << "\n";
            closesocket(sock);
            continue;
        }

        int totalSent = 0;
        while (totalSent < static_cast<int>(message.size()))
        {
            const int sent = send(
                sock,
                message.data() + totalSent,
                static_cast<int>(message.size()) - totalSent,
                0);
            if (sent <= 0)
            {
                std::cerr << "[Executor] TCP send failed: " << WSAGetLastError() << "\n";
                break;
            }
            totalSent += sent;
        }
        ok = totalSent == static_cast<int>(message.size());
        closesocket(sock);
        if (ok)
        {
            break;
        }
    }

    freeaddrinfo(result);
    return ok;
}

bool sendHttp(const SocketSendRequest &request)
{
    const auto parsed = parseUrl(request.destination, /*websocket=*/false);
    if (!parsed)
    {
        std::cerr << "[Executor] Invalid HTTP URL: " << request.destination << "\n";
        return false;
    }

    std::string method = request.httpMethod.empty() ? "POST" : request.httpMethod;
    for (char &c : method)
    {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    const std::wstring wmethod(method.begin(), method.end());

    HINTERNET session = WinHttpOpen(
        L"WheelTime/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!session)
    {
        std::cerr << "[Executor] WinHttpOpen failed: " << GetLastError() << "\n";
        return false;
    }

    WinHttpSetTimeouts(session, kNetworkTimeoutMs, kNetworkTimeoutMs, kNetworkTimeoutMs,
                       kNetworkTimeoutMs);

    HINTERNET connect = WinHttpConnect(session, parsed->host.c_str(), parsed->port, 0);
    if (!connect)
    {
        std::cerr << "[Executor] WinHttpConnect failed: " << GetLastError() << "\n";
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD flags = parsed->secure ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET requestHandle = WinHttpOpenRequest(
        connect,
        wmethod.c_str(),
        parsed->path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags);
    if (!requestHandle)
    {
        std::cerr << "[Executor] WinHttpOpenRequest failed: " << GetLastError() << "\n";
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return false;
    }

    const BOOL sent = WinHttpSendRequest(
        requestHandle,
        L"Content-Type: text/plain\r\n",
        static_cast<DWORD>(-1L),
        request.message.empty()
            ? WINHTTP_NO_REQUEST_DATA
            : const_cast<char *>(request.message.data()),
        static_cast<DWORD>(request.message.size()),
        static_cast<DWORD>(request.message.size()),
        0);

    bool ok = false;
    if (!sent)
    {
        std::cerr << "[Executor] WinHttpSendRequest failed: " << GetLastError() << "\n";
    }
    else if (!WinHttpReceiveResponse(requestHandle, nullptr))
    {
        std::cerr << "[Executor] WinHttpReceiveResponse failed: " << GetLastError() << "\n";
    }
    else
    {
        // Drain response body so the connection closes cleanly.
        DWORD available = 0;
        while (WinHttpQueryDataAvailable(requestHandle, &available) && available > 0)
        {
            std::vector<char> buffer(available);
            DWORD read = 0;
            if (!WinHttpReadData(requestHandle, buffer.data(), available, &read) || read == 0)
            {
                break;
            }
        }
        ok = true;
    }

    WinHttpCloseHandle(requestHandle);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return ok;
}

bool sendWebSocket(const SocketSendRequest &request)
{
    const auto parsed = parseUrl(request.destination, /*websocket=*/true);
    if (!parsed)
    {
        std::cerr << "[Executor] Invalid WebSocket URL: " << request.destination << "\n";
        return false;
    }

    HINTERNET session = WinHttpOpen(
        L"WheelTime/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!session)
    {
        std::cerr << "[Executor] WinHttpOpen failed: " << GetLastError() << "\n";
        return false;
    }

    WinHttpSetTimeouts(session, kNetworkTimeoutMs, kNetworkTimeoutMs, kNetworkTimeoutMs,
                       kNetworkTimeoutMs);

    HINTERNET connect = WinHttpConnect(session, parsed->host.c_str(), parsed->port, 0);
    if (!connect)
    {
        std::cerr << "[Executor] WinHttpConnect failed: " << GetLastError() << "\n";
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD flags = WINHTTP_FLAG_SECURE;
    if (!parsed->secure)
    {
        flags = 0;
    }

    HINTERNET requestHandle = WinHttpOpenRequest(
        connect,
        L"GET",
        parsed->path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags);
    if (!requestHandle)
    {
        std::cerr << "[Executor] WinHttpOpenRequest failed: " << GetLastError() << "\n";
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return false;
    }

    if (!WinHttpSetOption(requestHandle, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0))
    {
        std::cerr << "[Executor] WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET failed: "
                  << GetLastError() << "\n";
        WinHttpCloseHandle(requestHandle);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return false;
    }

    if (!WinHttpSendRequest(requestHandle, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        || !WinHttpReceiveResponse(requestHandle, nullptr))
    {
        std::cerr << "[Executor] WebSocket handshake request failed: " << GetLastError()
                  << "\n";
        WinHttpCloseHandle(requestHandle);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return false;
    }

    HINTERNET webSocket = WinHttpWebSocketCompleteUpgrade(requestHandle, 0);
    WinHttpCloseHandle(requestHandle);
    requestHandle = nullptr;
    if (!webSocket)
    {
        std::cerr << "[Executor] WinHttpWebSocketCompleteUpgrade failed: " << GetLastError()
                  << "\n";
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return false;
    }

    const DWORD sendStatus = WinHttpWebSocketSend(
        webSocket,
        WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
        request.message.empty()
            ? nullptr
            : reinterpret_cast<PVOID>(const_cast<char *>(request.message.data())),
        static_cast<DWORD>(request.message.size()));

    bool ok = sendStatus == NO_ERROR;
    if (!ok)
    {
        std::cerr << "[Executor] WinHttpWebSocketSend failed: " << sendStatus << "\n";
    }

    WinHttpWebSocketClose(webSocket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr, 0);
    WinHttpCloseHandle(webSocket);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return ok;
}

} // namespace

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

    void executeLaunchPreset(const std::string &presetId)
    {
        if (presetId == "browser")
        {
            executeScript("https://www.google.com");
            return;
        }
        if (presetId == "explorer")
        {
            executeScript("explorer.exe");
            return;
        }
        if (presetId == "calculator")
        {
            executeScript("calc.exe");
            return;
        }
        if (presetId == "notepad")
        {
            executeScript("notepad.exe");
            return;
        }
        if (presetId == "paint")
        {
            executeScript("mspaint.exe");
            return;
        }
        if (presetId == "taskmgr")
        {
            executeScript("taskmgr.exe");
            return;
        }

        std::cerr << "Unknown launch preset: " << presetId << "\n";
    }

    void keyDown(InputBind key)
    {
        std::vector<INPUT> inputs;
        inputs.reserve(5);
        appendModifierDowns(inputs, key.mod);
        pushKeyboardInput(inputs, static_cast<WORD>(key.input));
        sendInputs(inputs);
    }

    void keyUp(InputBind key)
    {
        std::vector<INPUT> inputs;
        inputs.reserve(5);
        pushKeyboardInput(inputs, static_cast<WORD>(key.input), KEYEVENTF_KEYUP);
        appendModifierUps(inputs, key.mod);
        sendInputs(inputs);
    }

    void modifiersDown(int mod)
    {
        if (mod == 0)
        {
            return;
        }
        std::vector<INPUT> inputs;
        inputs.reserve(4);
        appendModifierDowns(inputs, mod);
        sendInputs(inputs);
    }

    void modifiersUp(int mod)
    {
        if (mod == 0)
        {
            return;
        }
        std::vector<INPUT> inputs;
        inputs.reserve(8);
        appendModifierUps(inputs, mod);
        sendInputs(inputs);
    }

    void executeKey(InputBind key)
    {
        keyDown(key);
        keyUp(key);
    }

    void mouseButton(int button, bool down)
    {
        DWORD flag = 0;
        switch (button)
        {
        case 0:
            flag = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
            break;
        case 1:
            flag = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
            break;
        case 2:
            flag = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
            break;
        default:
            std::cerr << "[Executor] Unsupported mouse button: " << button << "\n";
            return;
        }

        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = flag;
        std::vector<INPUT> inputs{input};
        sendInputs(inputs);
    }

    bool sendSocket(const SocketSendRequest &request)
    {
        switch (request.protocol)
        {
        case SocketProtocol::Udp:
        case SocketProtocol::Tcp:
        {
            const auto hp = parseHostPort(request.destination);
            if (!hp)
            {
                std::cerr << "[Executor] Invalid host:port destination: "
                          << request.destination << "\n";
                return false;
            }
            const int type =
                request.protocol == SocketProtocol::Udp ? SOCK_DGRAM : SOCK_STREAM;
            return sendViaSocket(type, *hp, request.message);
        }
        case SocketProtocol::Http:
            return sendHttp(request);
        case SocketProtocol::WebSocket:
            return sendWebSocket(request);
        }
        std::cerr << "[Executor] Unknown socket protocol\n";
        return false;
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

void Executor::executeLaunchPreset(std::string presetId)
{
    m_impl->executeLaunchPreset(presetId);
}

void Executor::executeKey(InputBind key)
{
    m_impl->executeKey(key);
}

void Executor::keyDown(InputBind key)
{
    m_impl->keyDown(key);
}

void Executor::keyUp(InputBind key)
{
    m_impl->keyUp(key);
}

void Executor::modifiersDown(int mod)
{
    m_impl->modifiersDown(mod);
}

void Executor::modifiersUp(int mod)
{
    m_impl->modifiersUp(mod);
}

void Executor::mouseButton(int button, bool down)
{
    m_impl->mouseButton(button, down);
}

bool Executor::sendSocket(const SocketSendRequest &request)
{
    return m_impl->sendSocket(request);
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
