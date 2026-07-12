/**
 * @file ExecuteResult.hpp
 * @author your name (you@domain.com)
 * @brief Contains the result from after an action item executes, requeue or continue forth
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once
#include <chrono>

class ExecuteResult
{
public:
    enum class Type
    {
        Continue,
        DelayUntil
    };

    static ExecuteResult Continue();

    static ExecuteResult DelayUntil(
        std::chrono::steady_clock::time_point wakeTime);

    Type type() const;

    std::chrono::steady_clock::time_point wakeTime() const;

private:
    ExecuteResult(
        Type type,
        std::chrono::steady_clock::time_point wakeTime);

    Type m_type;

    std::chrono::steady_clock::time_point m_wakeTime;
};