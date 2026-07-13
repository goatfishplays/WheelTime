/**
 * @file SynchronousActionRunner.hpp
 * @brief Temporary calling-thread runner until the app switches to Scheduler.
 */

#pragma once

#include <memory>

#include "App/Action.hpp"

namespace Application
{

/**
 * @brief Runs an Action synchronously via ActionExecutionContext.
 *
 * Used by the live app until call sites move to Scheduler::submit.
 *
 * Behavior (legacy-compatible):
 * - Executes items on the calling thread.
 * - Sleeps on DelayUntil (workers in the real pool never do this).
 * - Recursively runs nested scheduleAction requests after waiting for their
 *   wake times.
 *
 * Prefer Scheduler for new code paths; keep this for UI-thread automation
 * until that migration is done.
 */
class SynchronousActionRunner
{
public:
    /**
     * @brief Takes ownership of @p action and runs it to completion (or cancel).
     */
    void run(std::unique_ptr<Action> action);

    /**
     * @brief Copies @p action and runs the copy (library Actions stay intact).
     */
    void run(const Action &action);
};

} // namespace Application
