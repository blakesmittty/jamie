#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

template <typename T>
class ThreadSafeQueue
{
    std::queue<T> q;
    std::mutex m;
    std::condition_variable cv;

public:
    void push(const T &value)
    {
        std::lock_guard<std::mutex> lock(m);
        q.push(value);
        cv.notify_one();
    }

    T pop()
    {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [&]
                { return !q.empty(); });
        T val = q.front();
        q.pop();
        return val;
    }

    bool empty()
    {
        std::lock_guard<std::mutex> lock(m);
        return q.empty();
    }
};
