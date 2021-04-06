#include "threadmanager.h"

#include <iostream>

static constexpr unsigned int maxJoiningThreads = 4;

ThreadManager::~ThreadManager() {
    assert(threads.empty());
}

void ThreadManager::enqueueJoin(std::thread &&thread) {
    threads.emplace_back(std::move(thread));
    if (threads.size() > maxJoiningThreads) {
        std::cout << "Joining thread " << threads.front().get_id() << "..." << std::endl;
        threads.front().join();
        threads.pop_front();
        std::cout << "Joined thread " << threads.front().get_id() << std::endl;
    }
}

void ThreadManager::joinAll() {
    for (std::thread &thread : threads) {
        std::thread::id id = thread.get_id();
        std::cout << "Joining thread " << id << "..." << std::endl;
        thread.join();
        std::cout << "Joined thread " << id << std::endl;
    }
    threads.clear();
}
