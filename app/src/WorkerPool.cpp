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

bool WorkerPool::waitResult(WorkerResult &out)
{
    return m_outbound.pop(out);
}

void WorkerPool::stop()
{
    m_inbound.stop();
    m_workers.clear();
    m_outbound.stop();
}

void WorkerPool::emit(WorkerResult result)
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
    while (m_inbound.pop(context))
    {
        WorkerResult result;
        result.context = std::move(context);

        while (!result.context->finished() && !result.context->isCancelled())
        {
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
                emit(std::move(result));
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
                emit(std::move(result));
                goto next_job;
            }
        }

        result.status = result.context->isCancelled()
                            ? WorkerResult::Status::Cancelled
                            : WorkerResult::Status::Completed;
        emit(std::move(result));

    next_job:
        continue;
    }
}

} // namespace Application
