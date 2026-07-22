/**
 * @file TestFakes.hpp
 * @brief Shared ActionItem fakes for scheduler and related tests.
 */

#pragma once

#include "App/Action.hpp"
#include "App/ActionExecutionContext.hpp"
#include "App/ActionItems/ActionItem.hpp"
#include "App/ExecuteResult.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace Application::TestFakes
{

class CountingItem : public ActionItem
{
public:
    explicit CountingItem(std::atomic<int> *counter)
        : m_counter{counter}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<CountingItem>(m_counter);
    }

    ExecuteResult execute(ActionExecutionContext & /*context*/) override
    {
        if (m_counter != nullptr)
        {
            m_counter->fetch_add(1, std::memory_order_acq_rel);
        }
        return ExecuteResult::Continue();
    }

private:
    std::atomic<int> *m_counter = nullptr;
};

class DelayItem : public ActionItem
{
public:
    explicit DelayItem(std::chrono::milliseconds delay)
        : m_delay{delay}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<DelayItem>(m_delay);
    }

    ExecuteResult execute(ActionExecutionContext & /*context*/) override
    {
        return ExecuteResult::DelayUntil(std::chrono::steady_clock::now() + m_delay);
    }

private:
    std::chrono::milliseconds m_delay;
};

class GateItem : public ActionItem
{
public:
    GateItem(std::atomic<int> *entered, std::atomic<bool> *release)
        : m_entered{entered}
        , m_release{release}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<GateItem>(m_entered, m_release);
    }

    ExecuteResult execute(ActionExecutionContext & /*context*/) override
    {
        m_entered->fetch_add(1, std::memory_order_acq_rel);
        while (!m_release->load(std::memory_order_acquire))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return ExecuteResult::Continue();
    }

private:
    std::atomic<int> *m_entered = nullptr;
    std::atomic<bool> *m_release = nullptr;
};

class RecordItem : public ActionItem
{
public:
    RecordItem(std::mutex *mutex, std::vector<int> *order, int id,
               std::chrono::milliseconds spin = std::chrono::milliseconds(1))
        : m_mutex{mutex}
        , m_order{order}
        , m_id{id}
        , m_spin{spin}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<RecordItem>(m_mutex, m_order, m_id, m_spin);
    }

    ExecuteResult execute(ActionExecutionContext & /*context*/) override
    {
        if (m_spin.count() > 0)
        {
            std::this_thread::sleep_for(m_spin);
        }
        std::lock_guard lock{*m_mutex};
        m_order->push_back(m_id);
        return ExecuteResult::Continue();
    }

private:
    std::mutex *m_mutex = nullptr;
    std::vector<int> *m_order = nullptr;
    int m_id = 0;
    std::chrono::milliseconds m_spin;
};

class TimestampItem : public ActionItem
{
public:
    TimestampItem(std::mutex *mutex, std::vector<std::chrono::steady_clock::time_point> *stamps)
        : m_mutex{mutex}
        , m_stamps{stamps}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<TimestampItem>(m_mutex, m_stamps);
    }

    ExecuteResult execute(ActionExecutionContext & /*context*/) override
    {
        const auto now = std::chrono::steady_clock::now();
        std::lock_guard lock{*m_mutex};
        m_stamps->push_back(now);
        return ExecuteResult::Continue();
    }

private:
    std::mutex *m_mutex = nullptr;
    std::vector<std::chrono::steady_clock::time_point> *m_stamps = nullptr;
};

class RegisterFlushItem : public ActionItem
{
public:
    explicit RegisterFlushItem(std::atomic<int> *flushCounter)
        : m_flushCounter{flushCounter}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<RegisterFlushItem>(m_flushCounter);
    }

    ExecuteResult execute(ActionExecutionContext &context) override
    {
        std::vector<std::unique_ptr<ActionItem>> flushItems;
        flushItems.push_back(std::make_unique<CountingItem>(m_flushCounter));
        auto flush = std::make_unique<Action>(std::move(flushItems), "flush", "", "flush", 0);
        flush->setCancelable(false);
        context.setCancelFlush(std::move(flush));
        return ExecuteResult::Continue();
    }

private:
    std::atomic<int> *m_flushCounter = nullptr;
};

class ScheduleChildItem : public ActionItem
{
public:
    ScheduleChildItem(std::atomic<int> *childCounter, uint32_t childChannel,
                      std::chrono::steady_clock::time_point wakeTime)
        : m_childCounter{childCounter}
        , m_childChannel{childChannel}
        , m_wakeTime{wakeTime}
    {
    }

    std::unique_ptr<ActionItem> clone() const override
    {
        return std::make_unique<ScheduleChildItem>(m_childCounter, m_childChannel, m_wakeTime);
    }

    ExecuteResult execute(ActionExecutionContext &context) override
    {
        std::vector<std::unique_ptr<ActionItem>> items;
        items.push_back(std::make_unique<CountingItem>(m_childCounter));
        auto child =
            std::make_unique<Action>(std::move(items), "child", "", "child", m_childChannel);
        context.scheduleAction(std::move(child), m_wakeTime);
        return ExecuteResult::Continue();
    }

private:
    std::atomic<int> *m_childCounter = nullptr;
    uint32_t m_childChannel = 0;
    std::chrono::steady_clock::time_point m_wakeTime{};
};

} // namespace Application::TestFakes
