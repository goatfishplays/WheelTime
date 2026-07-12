/**
 * @file DelayQueue.hpp
 * @brief Wake-time ordered queue of parked ActionExecutionContexts.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "App/ActionExecutionContext.hpp"

namespace Application
{

/**
 * @brief Min-heap of contexts waiting until a steady-clock wake time.
 *
 * Used by the Scheduler so workers never sleep for delays. The scheduler
 * parks Delayed (or not-yet-started) contexts here and sleeps on its mailbox
 * until the earliest wake, a new event, or stop.
 *
 * Not thread-safe — owned and used only by the scheduler thread.
 */
class DelayQueue
{
public:
    DelayQueue() = default;

    DelayQueue(const DelayQueue &) = delete;
    DelayQueue &operator=(const DelayQueue &) = delete;

    DelayQueue(DelayQueue &&) noexcept = default;
    DelayQueue &operator=(DelayQueue &&) noexcept = default;

    /**
     * @brief Parks @p context until @p wakeTime (earliest-first ordering).
     * Ignored if @p context is null.
     */
    void push(std::chrono::steady_clock::time_point wakeTime,
              std::unique_ptr<ActionExecutionContext> context);

    /**
     * @brief Removes and returns every context whose wake time is <= @p now.
     * Results are ordered earliest wake first (stable for equal times).
     */
    [[nodiscard]] std::vector<std::unique_ptr<ActionExecutionContext>> popDue(
        std::chrono::steady_clock::time_point now);

    /// @brief Earliest pending wake time, if the queue is non-empty.
    [[nodiscard]] std::optional<std::chrono::steady_clock::time_point> nextWakeTime() const;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    /// @brief Drops all parked contexts.
    void clear() noexcept;

private:
    struct Entry
    {
        std::chrono::steady_clock::time_point wakeTime{};
        std::unique_ptr<ActionExecutionContext> context;
        std::uint64_t sequence = 0; ///< Tie-break so equal wakes stay FIFO.
    };

    struct EarliestFirst
    {
        bool operator()(const Entry &a, const Entry &b) const noexcept;
    };

    std::vector<Entry> m_heap;
    std::uint64_t m_nextSequence = 0;
};

} // namespace Application
