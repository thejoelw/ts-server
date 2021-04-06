#pragma once

#include <thread>
#include <deque>

class ThreadManager {
public:
    static ThreadManager &getInstance() {
        static ThreadManager inst;
        return inst;
    }

    ~ThreadManager();

    void enqueueJoin(std::thread &&thread);
    void joinAll();

private:
    std::deque<std::thread> threads;
};
