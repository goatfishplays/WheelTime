/**
 * @file Scheduler.hpp
 * @brief Scheduler stubs (Phase 4+). Request recording types live on ActionExecutionContext.
 */

#pragma once

namespace Application
{

// TODO: Scheduler itself is not implemented yet (Phase 4).
enum class SchedulerRequestType
{
    ScheduleAction,
    CancelAction,
    CancelChannel
};

} // namespace Application
