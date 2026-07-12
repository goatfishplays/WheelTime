#include "App/WorkQueue.hpp"
#include "App/SynchronousActionRunner.hpp"

using namespace Application;

WorkQueue::WorkQueue()
{

    WorkQueue queue;

    constexpr int workerCount = 1; // TODO: test with parallel, should work, but some things might get scuffed

    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    // Create workers
    for (int i = 0; i < workerCount; ++i)
    {
        workers.emplace_back([&queue, i]
                             {
            Action action;
 
            while (queue.pop(action))
            {
                std::cout << "Worker " << i << " executing job\n";
                SynchronousActionRunner{}.run(std::make_unique<Action>(std::move(action)));
            }

            std::cout << "Worker " << i << " exiting\n"; });

        // while (!context.finished()) // TODO: final form something like this
        // {
        //     auto result =
        //         context.currentItem()->execute(context);

        //     if (result.type() == DelayUntil)
        //     {
        //         scheduler.returnContext(
        //             std::move(context),
        //             result);

        //         return;
        //     }

        //     context.advance();
        // }

        // scheduler.actionFinished(std::move(context));
    }

    // Add some work
    for (int i = 1; i <= 20; ++i)
    {
        queue.push([i]
                   {
            std::cout << "  Job " << i
                      << " running on thread "
                      << std::this_thread::get_id()
                      << '\n';

            std::this_thread::sleep_for(std::chrono::milliseconds(250)); });
    }

    // Tell workers to exit once the queue is empty.
    queue.stop();

    // Wait for every worker.
    for (auto &worker : workers)
    {
        worker.join();
    }
}

WorkQueue::~WorkQueue()
{
    stop();
}

void WorkQueue::push(Action action)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(action));
    }

    // Wake one waiting worker.
    m_cv.notify_one();
}

bool WorkQueue::pop(Action &action)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    // Sleep until there is work or we're stopping.
    m_cv.wait(lock, [this]
              { return !m_queue.empty() || m_stop; });

    if (m_stop && m_queue.empty())
        return false;

    action = std::move(m_queue.front());
    m_queue.pop();

    return true;
}

void WorkQueue::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::queue<Action> empty;
    m_queue.swap(empty);
}

void WorkQueue::stop()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }

    // Wake everyone so they can exit.
    m_cv.notify_all();
}
