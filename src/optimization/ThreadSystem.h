/*
    Small thread pool header: blocking parallelFor and job queue.
*/

#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <algorithm>

//--SMALL-THREAD-POOL--
//Tiny fixed-size thread pool with a blocking parallelFor. Keeps things simple and predictable.
class ThreadSystem
{
public:
    explicit ThreadSystem(int threads = 0)
    {
        const int hardware = (int)std::thread::hardware_concurrency();
        n = std::max(1, threads > 0 ? threads : (hardware > 1 ? hardware - 1 : 1)); //Leave one core for OS/UI if possible.

        stop = false;
        workers.reserve(n);

        for (int i = 0; i < n; ++i)
        {
            workers.emplace_back([this]
            {
                for (;;)
                {
                    std::function<void()> job;
                    {
                        std::unique_lock<std::mutex> lock(mutex);

                        conditionVariable.wait(lock, [this] { return stop || !q.empty(); }); //Sleep until work or shutdown.
                        if (stop && q.empty()) return;

                        job = std::move(q.front());
                        q.pop();
                    }

                    job(); //Run job outside lock.
                }
            });
        }
    }

    ~ThreadSystem()
    {
        {
            std::lock_guard<std::mutex> lk(mutex);
            stop = true; //Signal shutdown.
        }

        conditionVariable.notify_all(); //Wake all workers.
        for (auto& t : workers) if (t.joinable()) t.join();
    }

    int getThreadCount() const { return n; } //Current worker count.

    //Blocking parallelFor that splits [begin,end) into 'chunks' and waits for completion.
    template<typename Fn>
    void parallelFor(int begin, int end, int minGrain, Fn&& fn)
    {
        const int total = std::max(0, end - begin);

        if (total <= 0) return;

        const int maxChunks = std::max(1, std::min(n, total / std::max(1, minGrain)));
        const int chunks = std::max(1, maxChunks);

        struct Sync { std::atomic<int> remaining{ 0 }; std::mutex mutex; std::condition_variable conditionVariable; } sync;
        sync.remaining.store(chunks, std::memory_order_relaxed);

        for (int k = 0; k < chunks; ++k)
        {
            const int i0 = begin + (int)((int64_t)total * k / chunks);
            const int i1 = begin + (int)((int64_t)total * (k + 1) / chunks);

            enqueue([i0, i1, k, &fn, &sync]
            {
                fn(i0, i1, k); //User function receives range and chunk index.

                if (sync.remaining.fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    std::lock_guard<std::mutex> lk(sync.mutex);
                    sync.conditionVariable.notify_one(); //If the last chunk is finished, wake waiter.
                }
            });
        }

        std::unique_lock<std::mutex> lock(sync.mutex);
        sync.conditionVariable.wait(lock, [&sync] { return sync.remaining.load(std::memory_order_acquire) == 0; }); //Wait for all chunks.
    }

private:
    void enqueue(std::function<void()> job)
    {
        {
            std::lock_guard<std::mutex> lk(mutex);
            q.push(std::move(job));
        }

        conditionVariable.notify_one();
    }

    int n = 1;
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> q;
    std::mutex mutex;
    std::condition_variable conditionVariable;
    bool stop = false;
};
//--SMALL-THREAD-POOL-END--