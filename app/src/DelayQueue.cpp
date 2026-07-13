/**
 * @file DelayQueue.cpp
 * @brief DelayQueue definitions.
 */

#include "App/DelayQueue.hpp"

#include <algorithm>
#include <utility>

namespace Application
{

bool DelayQueue::EarliestFirst::operator()(const Entry &a, const Entry &b) const noexcept
{
    if (a.wakeTime != b.wakeTime)
    {
        return a.wakeTime > b.wakeTime; // min-heap: earliest at front after push_heap
    }
    return a.sequence > b.sequence;
}

void DelayQueue::rebuildHeap()
{
    std::make_heap(m_heap.begin(), m_heap.end(), EarliestFirst{});
}

void DelayQueue::push(std::chrono::steady_clock::time_point wakeTime,
                      std::unique_ptr<ActionExecutionContext> context)
{
    if (!context)
    {
        return;
    }

    Entry entry;
    entry.wakeTime = wakeTime;
    entry.context = std::move(context);
    entry.sequence = m_nextSequence++;
    m_heap.push_back(std::move(entry));
    std::push_heap(m_heap.begin(), m_heap.end(), EarliestFirst{});
}

std::vector<std::unique_ptr<ActionExecutionContext>> DelayQueue::popDue(
    std::chrono::steady_clock::time_point now)
{
    std::vector<std::unique_ptr<ActionExecutionContext>> due;
    while (!m_heap.empty() && m_heap.front().wakeTime <= now)
    {
        std::pop_heap(m_heap.begin(), m_heap.end(), EarliestFirst{});
        Entry entry = std::move(m_heap.back());
        m_heap.pop_back();
        due.push_back(std::move(entry.context));
    }
    return due;
}

std::vector<std::unique_ptr<ActionExecutionContext>> DelayQueue::removeIf(
    const std::function<bool(const ActionExecutionContext &)> &predicate)
{
    std::vector<std::unique_ptr<ActionExecutionContext>> removed;
    std::vector<Entry> kept;
    kept.reserve(m_heap.size());

    for (Entry &entry : m_heap)
    {
        if (entry.context && predicate(*entry.context))
        {
            removed.push_back(std::move(entry.context));
        }
        else
        {
            kept.push_back(std::move(entry));
        }
    }

    m_heap = std::move(kept);
    rebuildHeap();
    return removed;
}

std::optional<std::chrono::steady_clock::time_point> DelayQueue::nextWakeTime() const
{
    if (m_heap.empty())
    {
        return std::nullopt;
    }
    return m_heap.front().wakeTime;
}

bool DelayQueue::empty() const noexcept
{
    return m_heap.empty();
}

std::size_t DelayQueue::size() const noexcept
{
    return m_heap.size();
}

void DelayQueue::clear() noexcept
{
    m_heap.clear();
}

void DelayQueue::shiftAll(std::chrono::steady_clock::duration delta)
{
    if (delta == std::chrono::steady_clock::duration::zero() || m_heap.empty())
    {
        return;
    }

    for (Entry &entry : m_heap)
    {
        entry.wakeTime += delta;
    }
    rebuildHeap();
}

} // namespace Application
