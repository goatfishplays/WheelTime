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
#include <chrono>

namespace Application
{
    /**
     * @brief Contains the result from after an action item executes, requeue or continue forth
     *
     */
    class ExecuteResult
    {
    public:
        enum class Status
        {
            Continue,
            Delay
        };

        static ExecuteResult Continue();

        static ExecuteResult Delay(
            std::chrono::steady_clock::time_point wakeTime);

        Status status() const;

        std::chrono::steady_clock::time_point wakeTime() const;

    private:
        Status m_status;

        std::chrono::steady_clock::time_point m_wakeTime;
    };
}