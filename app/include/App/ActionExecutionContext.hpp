/**
 * @file ActionExecutionContext.hpp
 * @author your name (you@domain.com)
 * @brief Contains information for actions to be run
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <memory>
#include "App/Scheduler.hpp"

namespace Application
{

    class Action;
    class ActionItem;

    /**
     * @brief Holds an action to progress and run the action items of
     *
     */
    class ActionExecutionContext
    {
    public:
        explicit ActionExecutionContext(std::unique_ptr<Action> action);

        uint64_t actionId() const;
        uint32_t channel() const;

        bool isCancelled() const;

        /**
         * @brief Returns reference to the action
         *
         * @return Action&
         */
        Action &action();

        /**
         * @brief Returns the current action item
         *
         * @return ActionItem*
         */
        ActionItem *currentItem();

        /**
         * @brief Moves the current index forward(doesn't run it)
         *
         */
        void advance();

        /**
         * @brief Returns true when the entire action is finished running
         *
         * @return true
         * @return false
         */
        bool finished() const;

        void scheduleAction(
            std::unique_ptr<Action> action,
            std::chrono::steady_clock::time_point wakeTime);

        const std::vector<ScheduledAction> &
        scheduledActions() const;

        void clearScheduledActions();

    private:
        std::unique_ptr<Action> m_action;

        size_t m_currentIndex = 0;

        std::vector<ScheduledAction> m_scheduledActions;
    };
}