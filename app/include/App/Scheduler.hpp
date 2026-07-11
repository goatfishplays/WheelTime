#pragma once
#include <chrono>
#include <memory>
#include "App/Action.hpp"
namespace Application
{
    struct ScheduledItem
    {

        std::chrono::steady_clock::time_point executeAt;
        Action

            bool
            operator<(const ScheduledTask &other) const
        {
            // Reverse comparison because std::priority_queue is a max heap.
            return executeAt > other.executeAt;
        }
    };
}