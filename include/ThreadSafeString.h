#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

class ThreadSafeString {
    std::string str;
    std::mutex m;
    std::condition_variable cv;

public:
    void append(const std::string &piece) {
        {
            std::lock_guard<std::mutex> lock(m);
            str += piece;
        }
        cv.notify_one();
    }

    std::string pullString() {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [&]{return !str.find('.') != std::string::npos;});
        std::string s = str;
        str.clear();
        return s;

    }

};