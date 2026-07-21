/**
 * @file phase9_actionitems_smoke.cpp
 * @brief Disposable smoke tests for built-in ActionItems (Phase 9).
 */

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/ActionItems.hpp"
#include "App/App.hpp"
#include "App/ExecuteResult.hpp"

#include <Platform/Execute.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <QApplication>

#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace Application;

namespace
{

bool testDelay()
{
    AI_Delay delay{50};
    ActionExecutionContext ctx{std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "t", "", "t", 0)};
    const ExecuteResult result = delay.execute(ctx);
    if (result.type() != ExecuteResult::Type::DelayUntil)
    {
        std::cerr << "delay: expected DelayUntil\n";
        return false;
    }
    return true;
}

bool testKeystrokeHoldRegistersCleanup()
{
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<AI_Keystroke>('A', 0, 0.05f, true));
    auto action = std::make_unique<Action>(std::move(items), "hold", "", "hold", 1);
    ActionExecutionContext ctx{std::move(action)};

    ActionItem *item = ctx.currentItem();
    if (item == nullptr)
    {
        std::cerr << "hold: no item\n";
        return false;
    }

    const ExecuteResult result = item->execute(ctx);
    if (result.type() != ExecuteResult::Type::Continue)
    {
        std::cerr << "hold: expected Continue when proceed=true\n";
        return false;
    }
    if (!ctx.hasPendingSchedulerRequests())
    {
        std::cerr << "hold: expected cancel-flush / scheduleAction requests\n";
        return false;
    }

    auto flushes = ctx.takeCancelFlushes();
    auto scheduled = ctx.takeScheduledActions();
    if (flushes.empty() || scheduled.empty())
    {
        std::cerr << "hold: missing flush or scheduled release\n";
        return false;
    }
    if (flushes.front()->cancelable() || scheduled.front().action->cancelable())
    {
        std::cerr << "hold: cleanup Actions should be uncancelable\n";
        return false;
    }
    if (!scheduled.front().removeIfParentCancelled)
    {
        std::cerr << "hold: expected removeIfParentCancelled\n";
        return false;
    }
    return true;
}

bool testKeystrokeHoldBlocksWhenNotProceed()
{
    AI_Keystroke key{'B', 0, 0.05f, false};
    ActionExecutionContext ctx{std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "t", "", "t", 0)};
    const ExecuteResult result = key.execute(ctx);
    if (result.type() != ExecuteResult::Type::DelayUntil)
    {
        std::cerr << "hold_block: expected DelayUntil when proceed=false\n";
        return false;
    }
    if (!ctx.hasPendingSchedulerRequests())
    {
        std::cerr << "hold_block: expected cleanup registration\n";
        return false;
    }
    return true;
}

bool testMouseMoveAndButton()
{
    AI_MouseMove move{10, 20};
    AI_MouseButton button{1, 0.0f, false};
    ActionExecutionContext ctx{std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "t", "", "t", 0)};

    // MouseMove calls Platform setMousePos — skip execute in smoke.
    if (move.kind() != ActionItemKind::MouseMove || move.clone()->kind() != ActionItemKind::MouseMove)
    {
        std::cerr << "mouse_move: wrong kind/clone\n";
        return false;
    }
    if (move.x != 10 || move.y != 20)
    {
        std::cerr << "mouse_move: coords not stored\n";
        return false;
    }

    if (button.execute(ctx).type() != ExecuteResult::Type::Continue)
    {
        std::cerr << "mouse_button: expected Continue for tap\n";
        return false;
    }
    if (button.kind() != ActionItemKind::MouseButton)
    {
        std::cerr << "mouse_button: wrong kind\n";
        return false;
    }
    return true;
}

bool testMouseButtonHoldRegistersCleanup()
{
    std::vector<std::unique_ptr<ActionItem>> items;
    items.push_back(std::make_unique<AI_MouseButton>(0, 0.05f, true));
    auto action = std::make_unique<Action>(std::move(items), "mhold", "", "mhold", 1);
    ActionExecutionContext ctx{std::move(action)};

    ActionItem *item = ctx.currentItem();
    if (item == nullptr)
    {
        std::cerr << "mouse_hold: no item\n";
        return false;
    }

    const ExecuteResult result = item->execute(ctx);
    if (result.type() != ExecuteResult::Type::Continue)
    {
        std::cerr << "mouse_hold: expected Continue when proceed=true\n";
        return false;
    }
    if (!ctx.hasPendingSchedulerRequests())
    {
        std::cerr << "mouse_hold: expected cancel-flush / scheduleAction requests\n";
        return false;
    }

    auto flushes = ctx.takeCancelFlushes();
    auto scheduled = ctx.takeScheduledActions();
    if (flushes.empty() || scheduled.empty())
    {
        std::cerr << "mouse_hold: missing flush or scheduled release\n";
        return false;
    }
    if (flushes.front()->cancelable() || scheduled.front().action->cancelable())
    {
        std::cerr << "mouse_hold: cleanup Actions should be uncancelable\n";
        return false;
    }
    if (!scheduled.front().removeIfParentCancelled)
    {
        std::cerr << "mouse_hold: expected removeIfParentCancelled\n";
        return false;
    }
    return true;
}

bool testKeyRelease()
{
    AI_KeyRelease release{'Z', 0};
    ActionExecutionContext ctx{std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "t", "", "t", 0)};
    if (release.execute(ctx).type() != ExecuteResult::Type::Continue)
    {
        std::cerr << "key_release: expected Continue\n";
        return false;
    }
    return true;
}

bool testCancelItemsRecordRequests()
{
    ActionExecutionContext ctx{std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "t", "", "t", 3)};

    AI_Cancel cancelLatest{CancelLevel::MostRecent, 0};
    if (cancelLatest.execute(ctx).type() != ExecuteResult::Type::Continue)
    {
        std::cerr << "cancel_latest: expected Continue\n";
        return false;
    }
    {
        auto requests = ctx.takeCancelRequests();
        if (requests.size() != 1 || requests[0].level != PendingCancelRequest::Level::MostRecent
            || requests[0].excludeActionId != ctx.actionId() || requests[0].channel != 0)
        {
            std::cerr << "cancel_latest: bad pending request\n";
            return false;
        }
    }

    AI_Cancel cancelChannel{CancelLevel::Channel, 7};
    if (cancelChannel.execute(ctx).type() != ExecuteResult::Type::Continue)
    {
        std::cerr << "cancel_channel: expected Continue\n";
        return false;
    }
    {
        auto requests = ctx.takeCancelRequests();
        if (requests.size() != 1 || requests[0].level != PendingCancelRequest::Level::Channel
            || requests[0].channel != 7)
        {
            std::cerr << "cancel_channel: bad pending request\n";
            return false;
        }
    }

    AI_Cancel cancelAll{CancelLevel::All};
    if (cancelAll.kind() != ActionItemKind::Cancel || cancelAll.clone()->kind() != ActionItemKind::Cancel)
    {
        std::cerr << "cancel_all: wrong kind/clone\n";
        return false;
    }
    if (cancelAll.execute(ctx).type() != ExecuteResult::Type::Continue)
    {
        std::cerr << "cancel_all: expected Continue\n";
        return false;
    }
    {
        auto requests = ctx.takeCancelRequests();
        if (requests.size() != 1 || requests[0].level != PendingCancelRequest::Level::All)
        {
            std::cerr << "cancel_all: bad pending request\n";
            return false;
        }
    }

    AI_Cancel cancelLatestCh{CancelLevel::MostRecent, 4};
    (void)cancelLatestCh.execute(ctx);
    {
        auto requests = ctx.takeCancelRequests();
        if (requests.size() != 1 || requests[0].channel != 4
            || requests[0].level != PendingCancelRequest::Level::MostRecent)
        {
            std::cerr << "cancel_latest_ch: bad channel filter\n";
            return false;
        }
    }
    return true;
}

bool testNthItemDefaults()
{
    AI_NthRecent recent;
    AI_NthFrequent frequent;
    if (recent.n != 1 || frequent.n != 1)
    {
        std::cerr << "nth: expected default n=1\n";
        return false;
    }
    if (recent.kind() != ActionItemKind::NthRecent || frequent.kind() != ActionItemKind::NthFrequent)
    {
        std::cerr << "nth: wrong kind\n";
        return false;
    }
    if (recent.clone()->kind() != ActionItemKind::NthRecent
        || frequent.clone()->kind() != ActionItemKind::NthFrequent)
    {
        std::cerr << "nth: clone kind mismatch\n";
        return false;
    }
    return true;
}

constexpr int kSocketStressIterations = 50;
constexpr DWORD kSocketRecvTimeoutMs = 2000;

struct WinsockScope
{
    bool ok = false;

    WinsockScope()
    {
        WSADATA wsa{};
        ok = WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
    }

    ~WinsockScope()
    {
        if (ok)
        {
            WSACleanup();
        }
    }

    WinsockScope(const WinsockScope &) = delete;
    WinsockScope &operator=(const WinsockScope &) = delete;
};

bool bindLoopback(SOCKET sock, sockaddr_in &addr)
{
    addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0)
    {
        return false;
    }
    int addrLen = sizeof(addr);
    return getsockname(sock, reinterpret_cast<sockaddr *>(&addr), &addrLen) == 0;
}

std::string loopbackDestination(const sockaddr_in &addr)
{
    return "127.0.0.1:" + std::to_string(ntohs(addr.sin_port));
}

bool recvExactUdp(SOCKET listener, const std::string &expected)
{
    char buffer[512]{};
    sockaddr_in from{};
    int fromLen = sizeof(from);
    const int received = recvfrom(
        listener, buffer, static_cast<int>(sizeof(buffer)), 0,
        reinterpret_cast<sockaddr *>(&from), &fromLen);
    return received == static_cast<int>(expected.size())
        && std::memcmp(buffer, expected.data(), expected.size()) == 0;
}

bool recvExactTcp(SOCKET listener, const std::string &expected)
{
    SOCKET client = accept(listener, nullptr, nullptr);
    if (client == INVALID_SOCKET)
    {
        return false;
    }

    DWORD timeoutMs = kSocketRecvTimeoutMs;
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeoutMs),
               sizeof(timeoutMs));

    std::string received;
    received.resize(expected.size());
    int total = 0;
    while (total < static_cast<int>(expected.size()))
    {
        const int n = recv(
            client, received.data() + total,
            static_cast<int>(expected.size()) - total, 0);
        if (n <= 0)
        {
            closesocket(client);
            return false;
        }
        total += n;
    }
    closesocket(client);
    return received == expected;
}

bool testSocketInvalidDestination()
{
    Platform::Executor executor;
    Platform::SocketSendRequest request;
    request.protocol = Platform::SocketProtocol::Udp;
    request.destination = "not-a-host-port";
    request.message = "x";
    if (executor.sendSocket(request))
    {
        std::cerr << "socket_invalid: expected failure for bad destination\n";
        return false;
    }

    request.protocol = Platform::SocketProtocol::Http;
    request.destination = "ftp://example.com/";
    if (executor.sendSocket(request))
    {
        std::cerr << "socket_invalid: expected failure for bad http url\n";
        return false;
    }

    AI_Socket item{Platform::SocketProtocol::Udp, "", "hi"};
    if (item.kind() != ActionItemKind::Socket || item.clone()->kind() != ActionItemKind::Socket)
    {
        std::cerr << "socket_invalid: wrong kind/clone\n";
        return false;
    }
    return true;
}

bool testSocketUdpRoundTrip()
{
    WinsockScope winsock;
    if (!winsock.ok)
    {
        std::cerr << "udp: WSAStartup failed\n";
        return false;
    }

    SOCKET listener = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (listener == INVALID_SOCKET)
    {
        std::cerr << "udp: listener socket failed\n";
        return false;
    }

    sockaddr_in addr{};
    if (!bindLoopback(listener, addr))
    {
        std::cerr << "udp: bind failed\n";
        closesocket(listener);
        return false;
    }

    DWORD timeoutMs = kSocketRecvTimeoutMs;
    setsockopt(listener, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeoutMs),
               sizeof(timeoutMs));

    const std::string payload = "wheeltime-udp-smoke";
    const std::string destination = loopbackDestination(addr);

    Platform::Executor executor;
    Platform::SocketSendRequest request;
    request.protocol = Platform::SocketProtocol::Udp;
    request.destination = destination;
    request.message = payload;
    if (!executor.sendSocket(request))
    {
        std::cerr << "udp: sendSocket failed\n";
        closesocket(listener);
        return false;
    }

    const bool ok = recvExactUdp(listener, payload);
    closesocket(listener);
    if (!ok)
    {
        std::cerr << "udp: payload mismatch\n";
        return false;
    }
    return true;
}

bool testSocketTcpRoundTrip()
{
    WinsockScope winsock;
    if (!winsock.ok)
    {
        std::cerr << "tcp: WSAStartup failed\n";
        return false;
    }

    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET)
    {
        std::cerr << "tcp: listener socket failed\n";
        return false;
    }

    sockaddr_in addr{};
    if (!bindLoopback(listener, addr) || listen(listener, 1) != 0)
    {
        std::cerr << "tcp: bind/listen failed\n";
        closesocket(listener);
        return false;
    }

    DWORD timeoutMs = kSocketRecvTimeoutMs;
    setsockopt(listener, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeoutMs),
               sizeof(timeoutMs));

    const std::string payload = "wheeltime-tcp-smoke";
    const std::string destination = loopbackDestination(addr);

    Platform::Executor executor;
    Platform::SocketSendRequest request;
    request.protocol = Platform::SocketProtocol::Tcp;
    request.destination = destination;
    request.message = payload;
    if (!executor.sendSocket(request))
    {
        std::cerr << "tcp: sendSocket failed\n";
        closesocket(listener);
        return false;
    }

    const bool ok = recvExactTcp(listener, payload);
    closesocket(listener);
    if (!ok)
    {
        std::cerr << "tcp: payload mismatch\n";
        return false;
    }
    return true;
}

bool testSocketInvalidDestinationStress()
{
    Platform::Executor executor;
    for (int i = 0; i < kSocketStressIterations; ++i)
    {
        Platform::SocketSendRequest udp;
        udp.protocol = Platform::SocketProtocol::Udp;
        udp.destination = "not-a-host-port";
        udp.message = "stress-" + std::to_string(i);
        if (executor.sendSocket(udp))
        {
            std::cerr << "socket_invalid_stress: udp succeeded on iter " << i << '\n';
            return false;
        }

        Platform::SocketSendRequest tcp;
        tcp.protocol = Platform::SocketProtocol::Tcp;
        tcp.destination = "badhost";
        tcp.message = "x";
        if (executor.sendSocket(tcp))
        {
            std::cerr << "socket_invalid_stress: tcp succeeded on iter " << i << '\n';
            return false;
        }

        Platform::SocketSendRequest http;
        http.protocol = Platform::SocketProtocol::Http;
        http.destination = "ftp://example.com/";
        http.message = "x";
        if (executor.sendSocket(http))
        {
            std::cerr << "socket_invalid_stress: http succeeded on iter " << i << '\n';
            return false;
        }

        Platform::SocketSendRequest ws;
        ws.protocol = Platform::SocketProtocol::WebSocket;
        ws.destination = "not-a-ws-url";
        ws.message = "x";
        if (executor.sendSocket(ws))
        {
            std::cerr << "socket_invalid_stress: websocket succeeded on iter " << i << '\n';
            return false;
        }
    }
    return true;
}

bool testSocketUdpRoundTripStress()
{
    WinsockScope winsock;
    if (!winsock.ok)
    {
        std::cerr << "udp_stress: WSAStartup failed\n";
        return false;
    }

    SOCKET listener = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (listener == INVALID_SOCKET)
    {
        std::cerr << "udp_stress: listener socket failed\n";
        return false;
    }

    sockaddr_in addr{};
    if (!bindLoopback(listener, addr))
    {
        std::cerr << "udp_stress: bind failed\n";
        closesocket(listener);
        return false;
    }

    DWORD timeoutMs = kSocketRecvTimeoutMs;
    setsockopt(listener, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeoutMs),
               sizeof(timeoutMs));

    const std::string destination = loopbackDestination(addr);
    Platform::Executor executor;

    for (int i = 0; i < kSocketStressIterations; ++i)
    {
        const std::string payload = "udp-stress-" + std::to_string(i);
        Platform::SocketSendRequest request;
        request.protocol = Platform::SocketProtocol::Udp;
        request.destination = destination;
        request.message = payload;
        if (!executor.sendSocket(request))
        {
            std::cerr << "udp_stress: sendSocket failed on iter " << i << '\n';
            closesocket(listener);
            return false;
        }
        if (!recvExactUdp(listener, payload))
        {
            std::cerr << "udp_stress: payload mismatch on iter " << i << '\n';
            closesocket(listener);
            return false;
        }
    }

    closesocket(listener);
    return true;
}

bool testSocketTcpRoundTripStress()
{
    WinsockScope winsock;
    if (!winsock.ok)
    {
        std::cerr << "tcp_stress: WSAStartup failed\n";
        return false;
    }

    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET)
    {
        std::cerr << "tcp_stress: listener socket failed\n";
        return false;
    }

    sockaddr_in addr{};
    if (!bindLoopback(listener, addr) || listen(listener, SOMAXCONN) != 0)
    {
        std::cerr << "tcp_stress: bind/listen failed\n";
        closesocket(listener);
        return false;
    }

    DWORD timeoutMs = kSocketRecvTimeoutMs;
    setsockopt(listener, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeoutMs),
               sizeof(timeoutMs));

    const std::string destination = loopbackDestination(addr);
    Platform::Executor executor;

    for (int i = 0; i < kSocketStressIterations; ++i)
    {
        const std::string payload = "tcp-stress-" + std::to_string(i);
        Platform::SocketSendRequest request;
        request.protocol = Platform::SocketProtocol::Tcp;
        request.destination = destination;
        request.message = payload;
        if (!executor.sendSocket(request))
        {
            std::cerr << "tcp_stress: sendSocket failed on iter " << i << '\n';
            closesocket(listener);
            return false;
        }
        if (!recvExactTcp(listener, payload))
        {
            std::cerr << "tcp_stress: payload mismatch on iter " << i << '\n';
            closesocket(listener);
            return false;
        }
    }

    closesocket(listener);
    return true;
}

bool testSocketActionItemExecuteStress()
{
    WinsockScope winsock;
    if (!winsock.ok)
    {
        std::cerr << "ai_socket_stress: WSAStartup failed\n";
        return false;
    }

    SOCKET udpListener = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKET tcpListener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (udpListener == INVALID_SOCKET || tcpListener == INVALID_SOCKET)
    {
        std::cerr << "ai_socket_stress: listener sockets failed\n";
        if (udpListener != INVALID_SOCKET)
        {
            closesocket(udpListener);
        }
        if (tcpListener != INVALID_SOCKET)
        {
            closesocket(tcpListener);
        }
        return false;
    }

    sockaddr_in udpAddr{};
    sockaddr_in tcpAddr{};
    if (!bindLoopback(udpListener, udpAddr) || !bindLoopback(tcpListener, tcpAddr)
        || listen(tcpListener, SOMAXCONN) != 0)
    {
        std::cerr << "ai_socket_stress: bind/listen failed\n";
        closesocket(udpListener);
        closesocket(tcpListener);
        return false;
    }

    DWORD timeoutMs = kSocketRecvTimeoutMs;
    setsockopt(udpListener, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeoutMs),
               sizeof(timeoutMs));
    setsockopt(tcpListener, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeoutMs),
               sizeof(timeoutMs));

    const std::string udpDst = loopbackDestination(udpAddr);
    const std::string tcpDst = loopbackDestination(tcpAddr);
    ActionExecutionContext ctx{std::make_unique<Action>(
        std::vector<std::unique_ptr<ActionItem>>{}, "socket-stress", "", "socket-stress", 0)};

    for (int i = 0; i < kSocketStressIterations; ++i)
    {
        const std::string udpPayload = "ai-udp-" + std::to_string(i);
        AI_Socket udpItem{Platform::SocketProtocol::Udp, udpDst, udpPayload};
        if (udpItem.execute(ctx).type() != ExecuteResult::Type::Continue)
        {
            std::cerr << "ai_socket_stress: udp execute not Continue on iter " << i << '\n';
            closesocket(udpListener);
            closesocket(tcpListener);
            return false;
        }
        if (!recvExactUdp(udpListener, udpPayload))
        {
            std::cerr << "ai_socket_stress: udp payload mismatch on iter " << i << '\n';
            closesocket(udpListener);
            closesocket(tcpListener);
            return false;
        }

        const std::string tcpPayload = "ai-tcp-" + std::to_string(i);
        AI_Socket tcpItem{Platform::SocketProtocol::Tcp, tcpDst, tcpPayload};
        if (tcpItem.execute(ctx).type() != ExecuteResult::Type::Continue)
        {
            std::cerr << "ai_socket_stress: tcp execute not Continue on iter " << i << '\n';
            closesocket(udpListener);
            closesocket(tcpListener);
            return false;
        }
        if (!recvExactTcp(tcpListener, tcpPayload))
        {
            std::cerr << "ai_socket_stress: tcp payload mismatch on iter " << i << '\n';
            closesocket(udpListener);
            closesocket(tcpListener);
            return false;
        }

        // Failures still continue the action chain.
        AI_Socket bad{Platform::SocketProtocol::Udp, "not-a-host-port", "x"};
        if (bad.execute(ctx).type() != ExecuteResult::Type::Continue)
        {
            std::cerr << "ai_socket_stress: bad dest should still Continue on iter " << i << '\n';
            closesocket(udpListener);
            closesocket(tcpListener);
            return false;
        }

        auto cloned = udpItem.clone();
        if (cloned == nullptr || cloned->kind() != ActionItemKind::Socket)
        {
            std::cerr << "ai_socket_stress: clone failed on iter " << i << '\n';
            closesocket(udpListener);
            closesocket(tcpListener);
            return false;
        }
    }

    closesocket(udpListener);
    closesocket(tcpListener);
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication qtApp(argc, argv);
    (void)App::getInstance();

    using TestFn = bool (*)();
    const std::pair<const char *, TestFn> tests[] = {
        {"delay", testDelay},
        {"keystroke_hold_cleanup", testKeystrokeHoldRegistersCleanup},
        {"keystroke_hold_blocks", testKeystrokeHoldBlocksWhenNotProceed},
        {"mouse_move_button", testMouseMoveAndButton},
        {"mouse_button_hold_cleanup", testMouseButtonHoldRegistersCleanup},
        {"key_release", testKeyRelease},
        {"cancel_items", testCancelItemsRecordRequests},
        {"nth_item_defaults", testNthItemDefaults},
        {"socket_invalid_destination", testSocketInvalidDestination},
        {"socket_udp_roundtrip", testSocketUdpRoundTrip},
        {"socket_tcp_roundtrip", testSocketTcpRoundTrip},
        {"socket_invalid_destination_stress", testSocketInvalidDestinationStress},
        {"socket_udp_roundtrip_stress", testSocketUdpRoundTripStress},
        {"socket_tcp_roundtrip_stress", testSocketTcpRoundTripStress},
        {"socket_action_item_execute_stress", testSocketActionItemExecuteStress},
    };

    int failed = 0;
    for (const auto &[name, fn] : tests)
    {
        std::cout << "[RUN ] " << name << '\n';
        if (!fn())
        {
            std::cout << "[FAIL] " << name << '\n';
            ++failed;
        }
        else
        {
            std::cout << "[PASS] " << name << '\n';
        }
    }

    if (failed != 0)
    {
        std::cerr << failed << " phase9 actionitems smoke test(s) failed\n";
        return 1;
    }

    std::cout << "Phase 9 ActionItems smoke tests passed\n";
    return 0;
}
