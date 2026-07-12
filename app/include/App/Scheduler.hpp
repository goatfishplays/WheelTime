#pragma once
#include <chrono>
#include <memory>
#include "App/Action.hpp"
namespace Application
{
    // TODO: none of this does naything yet
    enum class SchedulerRequestType
    {
        ScheduleAction,
        CancelAction,
        CancelChannel
    };

    struct ScheduledAction
    {
        std::unique_ptr<Action> action;

        std::chrono::steady_clock::time_point wakeTime;
    };
}