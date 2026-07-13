#pragma once
#include "App/Action.hpp"
#include "App/ActionItems.hpp"
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <memory>

namespace Application
{
    struct WorkQueueItem
    {
        std::unique_ptr<ActionItem> currentActionItem;
        std::unique_ptr<ActionItem> nextActionItem;

        bool progress();
    };

    class WorkQueue
    {
    public:
        WorkQueue();
        ~WorkQueue();

        /**
         * @brief Places an action onto the work queue
         *
         * @param action
         */
        void push(Action action);

        /**
         * @brief Pops an item from the queue into the referenced action
         *
         * @param action
         * @return true
         * @return false Strangeness happened and failed to place in action
         */
        bool pop(Action &action);

        /**
         * @brief Clears the current action queue
         *
         * NOTE: the currently executing action will continue to completion
         *
         */
        void clear();

        /**
         * @brief End this object
         *
         */
        void stop();

    private:
        std::queue<Action> m_queue;
        std::mutex m_mutex;
        std::condition_variable m_cv;
        bool m_stop = false;
    };
}
