#include "threadmanager.h"

#include <iostream>
#include <sstream>
#include <assert.h>

static constexpr unsigned int maxJoiningThreads = 4;

ThreadManager::~ThreadManager() {
    assert(threads.empty());
}

void ThreadManager::enqueueJoin(std::thread &&thread) {
    threads.emplace_back(std::move(thread));
    if (threads.size() > maxJoiningThreads) {
        join(threads.front());
        threads.pop_front();
    }
}

void ThreadManager::joinAll() {
    for (std::thread &thread : threads) {
        join(thread);
    }
    threads.clear();
}

void ThreadManager::join(std::thread &thread) {
    std::thread::id id = thread.get_id();
    std::cout << "Joining thread " << id << "..." << std::endl;
    thread.join();
    std::cout << "Joined thread " << id << std::endl;
}
