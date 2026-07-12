/**
 * @file ThreadSafeQueue.hpp
 * @brief Reusable MPMC blocking queue for WorkerPool / Scheduler.
 */

#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

namespace Application
{

/**
 * @brief Result of a timed pop on ThreadSafeQueue.
 */
enum class QueueWaitStatus
{
    Item,    ///< An element was written to the out-parameter.
    Timeout, ///< Deadline passed with an empty queue.
    Stopped  ///< Queue is stopped and empty.
};

/**
 * @brief Multi-producer / multi-consumer FIFO queue.
 *
 * Suitable for worker inbound/outbound queues and the scheduler mailbox.
 *
 * Lifecycle:
 * - push / pop while running.
 * - stop() is permanent; further pushes are ignored.
 * - After stop(), consumers drain remaining items; then pop returns false /
 *   Stopped.
 */
template <typename T>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue() = default;

    ThreadSafeQueue(const ThreadSafeQueue &) = delete;
    ThreadSafeQueue &operator=(const ThreadSafeQueue &) = delete;

    ThreadSafeQueue(ThreadSafeQueue &&) = delete;
    ThreadSafeQueue &operator=(ThreadSafeQueue &&) = delete;

    ~ThreadSafeQueue()
    {
        stop();
    }

    /**
     * @brief Enqueues an item and wakes one waiter.
     * Ignored if the queue has already been stopped.
     */
    void push(T value)
    {
        {
            std::lock_guard lock{m_mutex};
            if (m_stopped)
            {
                return;
            }
            m_queue.push(std::move(value));
        }
        m_cv.notify_one();
    }

    /**
     * @brief Blocks until an item is available, or the queue is stopped and empty.
     * @param[out] out Receives the popped value on success.
     * @return true if @p out received an item; false if stopped and drained.
     */
    [[nodiscard]] bool pop(T &out)
    {
        std::unique_lock lock{m_mutex};
        m_cv.wait(lock, [this] { return !m_queue.empty() || m_stopped; });

        if (m_queue.empty())
        {
            return false;
        }

        out = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    /**
     * @brief Waits until an item is available, @p deadline passes, or the queue stops.
     * @param[out] out Receives the popped value when status is Item.
     * @param deadline Steady-clock time to wake if still empty.
     */
    [[nodiscard]] QueueWaitStatus popWaitUntil(
        T &out,
        std::chrono::steady_clock::time_point deadline)
    {
        std::unique_lock lock{m_mutex};
        while (m_queue.empty() && !m_stopped)
        {
            if (m_cv.wait_until(lock, deadline) == std::cv_status::timeout)
            {
                if (!m_queue.empty())
                {
                    break;
                }
                return QueueWaitStatus::Timeout;
            }
        }

        if (m_queue.empty())
        {
            return QueueWaitStatus::Stopped;
        }

        out = std::move(m_queue.front());
        m_queue.pop();
        return QueueWaitStatus::Item;
    }

    /**
     * @brief Discards all queued items.
     * In-flight pop waiters are not given discarded elements.
     */
    void clear()
    {
        std::lock_guard lock{m_mutex};
        std::queue<T> empty;
        m_queue.swap(empty);
    }

    /**
     * @brief Permanently stops the queue and wakes all waiters.
     * Remaining items can still be popped until the queue is drained.
     */
    void stop()
    {
        {
            std::lock_guard lock{m_mutex};
            m_stopped = true;
        }
        m_cv.notify_all();
    }

    /// @brief Whether stop() has been called.
    [[nodiscard]] bool stopped() const
    {
        std::lock_guard lock{m_mutex};
        return m_stopped;
    }

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<T> m_queue;
    bool m_stopped = false;
};

} // namespace Application
