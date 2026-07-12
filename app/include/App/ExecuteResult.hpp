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
 * Describes only the fate of the **current** Action after one ActionItem
 * returns. Nested work is requested via ActionExecutionContext::scheduleAction,
 * not via ExecuteResult.
 *
 * Workers: on DelayUntil, return the context to the scheduler (do not sleep).
 * Scheduler: park the context until wakeTime while holding its channel.
 */
class ExecuteResult
{
public:
    enum class Type
    {
        Continue,  ///< Keep running the next item on a worker (or finish if none remain).
        DelayUntil ///< Pause this Action until @ref wakeTime().
    };

    /// @brief Proceed with the current Action (advance / finish as appropriate).
    [[nodiscard]] static ExecuteResult Continue();

    /**
     * @brief Pause the current Action until @p wakeTime.
     * The item that returned this has already completed its work.
     */
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
