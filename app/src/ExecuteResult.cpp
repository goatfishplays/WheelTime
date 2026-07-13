/**
 * @file ExecuteResult.cpp
 * @brief ExecuteResult factory and accessor definitions.
 */

#include "App/ExecuteResult.hpp"

namespace Application
{

ExecuteResult::ExecuteResult(Type type, std::chrono::steady_clock::time_point wakeTime) noexcept
    : m_type{type}
    , m_wakeTime{wakeTime}
{
}

ExecuteResult ExecuteResult::Continue()
{
    return ExecuteResult{Type::Continue, {}};
}

ExecuteResult ExecuteResult::DelayUntil(std::chrono::steady_clock::time_point wakeTime)
{
    return ExecuteResult{Type::DelayUntil, wakeTime};
}

ExecuteResult::Type ExecuteResult::type() const noexcept
{
    return m_type;
}

std::chrono::steady_clock::time_point ExecuteResult::wakeTime() const noexcept
{
    return m_wakeTime;
}

} // namespace Application
