/**
 * @file Execute.hpp
 * @author goatfishplays@gmail.com
 * @brief Execute actions such as keystrokes, scripts, etc.
 * @version 0.1
 * @date 2026-06-30
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once
#include "Platform/Inputs.hpp"
#include <memory>
#include <string>

namespace Platform
{
    /// @brief Network send protocol for @ref Executor::sendSocket.
    enum class SocketProtocol
    {
        Udp,
        Tcp,
        Http,
        WebSocket
    };

    /**
     * @brief Parameters for a one-shot network send.
     *
     * Destination is `host:port` for Udp/Tcp, or a full URL for Http/WebSocket.
     * `httpMethod` is used only for Http (default POST).
     */
    struct SocketSendRequest
    {
        SocketProtocol protocol = SocketProtocol::Udp;
        std::string destination;
        std::string message;
        std::string httpMethod = "POST";
    };

    class Executor
    {
    public:
        Executor();
        ~Executor();
        void setMousePos(int x, int y);
        void executeKey(InputBind key);
        void keyDown(InputBind key);
        void keyUp(InputBind key);
        /// @brief Press modifier keys only (same mask as InputBind::mod).
        void modifiersDown(int mod);
        /// @brief Release modifier keys only (same mask as InputBind::mod).
        void modifiersUp(int mod);
        /// @brief Mouse button press/release. `button`: 0 left, 1 right, 2 middle.
        void mouseButton(int button, bool down);
        void executeScript(std::string filepath);
        /**
         * @brief Launches one of the built-in app presets using platform-specific resolution.
         *
         * Example preset IDs currently include `browser`, `explorer`,
         * `calculator`, `notepad`, `paint`, and `taskmgr`.
         */
        void executeLaunchPreset(std::string presetId);

        /**
         * @brief Synchronous one-shot send (UDP/TCP/HTTP/WebSocket).
         * @return false on failure; logs the reason. Never throws.
         */
        bool sendSocket(const SocketSendRequest &request);

    private:
        /**
         * @brief The OS specific parts
         *
         */
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
