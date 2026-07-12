/**
 * @file ExecuteResult.hpp
 * @brief Fate of the current Action after an ActionItem executes.
 */

#pragma once

#include <chrono>

namespace Application
{

/**
 * @brief Small result telling the scheduler whether to continue or delay.
 *
 * Describes only the current Action. Future variants may be added as needed.
 */
class ExecuteResult
{
public:
    enum class Type
    {
        Continue,
        DelayUntil
    };

    [[nodiscard]] static ExecuteResult Continue();

    [[nodiscard]] static ExecuteResult DelayUntil(
        std::chrono::steady_clock::time_point wakeTime);

    [[nodiscard]] Type type() const noexcept;

    /**
     * @brief Wake time for DelayUntil. Meaningless when type() is Continue.
     */
    [[nodiscard]] std::chrono::steady_clock::time_point wakeTime() const noexcept;

private:
    ExecuteResult(Type type, std::chrono::steady_clock::time_point wakeTime) noexcept;

    Type m_type;
    std::chrono::steady_clock::time_point m_wakeTime;
};

} // namespace Application
