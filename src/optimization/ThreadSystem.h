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
class ThreadSystem
{
public:
    explicit ThreadSystem(int threads = 0)
    {
        const int hw = (int)std::thread::hardware_concurrency();
        n = std::max(1, threads > 0 ? threads : (hw > 1 ? hw - 1 : 1));

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
                            std::unique_lock<std::mutex> lock(m);
                            cv.wait(lock, [this] { return stop || !q.empty(); });
                            if (stop && q.empty()) return;
                            job = std::move(q.front());
                            q.pop();
                        }
                        job();
                    }
                });
        }
    }

    ~ThreadSystem()
    {
        {
            std::lock_guard<std::mutex> lk(m);
            stop = true;
        }
        cv.notify_all();
        for (auto& t : workers) if (t.joinable()) t.join();
    }

    int threadCount() const { return n; }

    template<typename Fn>
    void parallel_for(int begin, int end, int minGrain, Fn&& fn)
    {
        const int total = std::max(0, end - begin);
        if (total <= 0) return;

        const int maxChunks = std::max(1, std::min(n, total / std::max(1, minGrain)));
        const int chunks = std::max(1, maxChunks);

        struct Sync { std::atomic<int> remaining{ 0 }; std::mutex m; std::condition_variable cv; } sync;
        sync.remaining.store(chunks, std::memory_order_relaxed);

        for (int k = 0; k < chunks; ++k)
        {
            const int i0 = begin + (int)((int64_t)total * k / chunks);
            const int i1 = begin + (int)((int64_t)total * (k + 1) / chunks);

            enqueue([i0, i1, k, &fn, &sync]
                {
                    fn(i0, i1, k);
                    if (sync.remaining.fetch_sub(1, std::memory_order_acq_rel) == 1)
                    {
                        std::lock_guard<std::mutex> lk(sync.m);
                        sync.cv.notify_one();
                    }
                });
        }

        std::unique_lock<std::mutex> lock(sync.m);
        sync.cv.wait(lock, [&sync] { return sync.remaining.load(std::memory_order_acquire) == 0; });
    }

private:
    void enqueue(std::function<void()> job)
    {
        {
            std::lock_guard<std::mutex> lk(m);
            q.push(std::move(job));
        }
        cv.notify_one();
    }

    int n = 1;
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> q;
    std::mutex m;
    std::condition_variable cv;
    bool stop = false;
};
//--SMALL-THREAD-POOL-END--