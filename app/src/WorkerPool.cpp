/**
 * @file WorkerPool.cpp
 * @brief WorkerPool definitions.
 */

#include "App/WorkerPool.hpp"

#include "App/ActionItems.hpp"
#include "App/ExecuteResult.hpp"

#include <utility>

namespace Application
{

WorkerPool::WorkerPool(std::size_t workerCount)
    : WorkerPool(workerCount, nullptr)
{
}

WorkerPool::WorkerPool(std::size_t workerCount, std::function<void(WorkerResult)> resultHandler)
    : m_resultHandler{std::move(resultHandler)}
{
    if (workerCount == 0)
    {
        workerCount = 1;
    }

    m_workers.reserve(workerCount);
    for (std::size_t i = 0; i < workerCount; ++i)
    {
        m_workers.emplace_back([this] { workerMain(); });
    }
}

WorkerPool::~WorkerPool()
{
    stop();
}

void WorkerPool::submit(std::unique_ptr<ActionExecutionContext> context)
{
    if (!context)
    {
        return;
    }
    m_inbound.push(std::move(context));
}

void WorkerPool::submitUrgent(std::unique_ptr<ActionExecutionContext> context)
{
    if (!context)
    {
        return;
    }
    m_urgent.push(std::move(context));
    m_pauseCv.notify_all();
}

void WorkerPool::setPaused(bool paused)
{
    {
        std::lock_guard lock{m_pauseMutex};
        m_paused.store(paused, std::memory_order_release);
    }
    m_pauseCv.notify_all();
}

bool WorkerPool::isPaused() const noexcept
{
    return m_paused.load(std::memory_order_acquire);
}

bool WorkerPool::waitResult(WorkerResult &out)
{
    return m_outbound.pop(out);
}

void WorkerPool::stop()
{
    setPaused(false);
    m_urgent.stop();
    m_inbound.stop();
    m_workers.clear();
    m_outbound.stop();
}

void WorkerPool::deliverResult(WorkerResult result)
{
    if (m_resultHandler)
    {
        m_resultHandler(std::move(result));
        return;
    }
    m_outbound.push(std::move(result));
}

void WorkerPool::workerMain()
{
    std::unique_ptr<ActionExecutionContext> context;
    for (;;)
    {
        bool urgentJob = false;

        // Cancel-flush and other urgent work bypass the pause gate.
        if (m_urgent.tryPop(context))
        {
            urgentJob = true;
        }
        else if (m_paused.load(std::memory_order_acquire))
        {
            std::unique_lock lock{m_pauseMutex};
            m_pauseCv.wait(lock, [this] {
                return !m_paused.load(std::memory_order_acquire) || !m_urgent.empty() ||
                       m_inbound.stopped();
            });
            if (m_inbound.stopped() && m_urgent.empty())
            {
                break;
            }
            continue;
        }
        else
        {
            // Timed pop so urgent work can be noticed without a dedicated waiter.
            const auto status = m_inbound.popWaitUntil(
                context, std::chrono::steady_clock::now() + std::chrono::milliseconds(25));
            if (status == QueueWaitStatus::Timeout)
            {
                continue;
            }
            if (status == QueueWaitStatus::Stopped)
            {
                if (m_urgent.tryPop(context))
                {
                    urgentJob = true;
                }
                else
                {
                    break;
                }
            }
        }

        WorkerResult result;
        result.context = std::move(context);

        while (!result.context->finished() && !result.context->isCancelled())
        {
            // Pause takes effect between ActionItems (current item always finishes).
            // Urgent cleanups run to completion even while the pool is paused.
            if (!urgentJob && m_paused.load(std::memory_order_acquire))
            {
                result.status = WorkerResult::Status::Continue;
                deliverResult(std::move(result));
                goto next_job;
            }

            ActionItem *item = result.context->currentItem();
            if (item == nullptr)
            {
                break;
            }

            const ExecuteResult executeResult = item->execute(*result.context);

            if (executeResult.type() == ExecuteResult::Type::DelayUntil)
            {
                result.context->advance();
                result.status = WorkerResult::Status::Delayed;
                result.wakeTime = executeResult.wakeTime();
                deliverResult(std::move(result));
                goto next_job;
            }

            result.context->advance();

            // Yield so the scheduler can ingest scheduleAction / setCancelFlush.
            if (result.context->hasPendingSchedulerRequests())
            {
                if (result.context->isCancelled())
                {
                    result.status = WorkerResult::Status::Cancelled;
                }
                else if (result.context->finished())
                {
                    result.status = WorkerResult::Status::Completed;
                }
                else
                {
                    result.status = WorkerResult::Status::Continue;
                }
                deliverResult(std::move(result));
                goto next_job;
            }
        }

        result.status = result.context->isCancelled()
                            ? WorkerResult::Status::Cancelled
                            : WorkerResult::Status::Completed;
        deliverResult(std::move(result));

    next_job:
        continue;
    }
}

} // namespace Application
