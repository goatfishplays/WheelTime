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
#include <Platform/Inputs.hpp>
#include <memory>
#include <string>

namespace Platform
{
    class Executor
    {
    public:
        Executor();
        ~Executor();
        void setMousePos(int x, int y);
        void executeKey(InputBind key);
        void executeScript(std::string filepath);

    private:
        /**
         * @brief The OS specific parts
         *
         */
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
}