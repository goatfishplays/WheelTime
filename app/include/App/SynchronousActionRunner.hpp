/**
 * @file SynchronousActionRunner.hpp
 * @brief Temporary calling-thread runner until the real scheduler exists.
 */

#pragma once

#include <memory>

#include "App/Action.hpp"

namespace Application
{

/**
 * @brief Runs an Action synchronously via ActionExecutionContext.
 *
 * Preserves pre-scheduler behavior (including sleeping on DelayUntil) so the
 * app keeps working. Replace call sites with Scheduler::submit once Phase 4+.
 */
class SynchronousActionRunner
{
public:
    void run(std::unique_ptr<Action> action);
    void run(const Action &action);
};

} // namespace Application
