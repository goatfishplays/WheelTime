/**
 * @file ThreadSafeQueue.hpp
 * @brief Reusable MPMC blocking queue for WorkerPool / Scheduler.
 */

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

namespace Application
{

/**
 * @brief Multi-producer / multi-consumer FIFO queue.
 *
 * pop() blocks until an item is available or the queue has been stopped and
 * drained. stop() is permanent for the lifetime of the instance.
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

    /// @brief Discards all queued items. In-flight pops are unaffected.
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
