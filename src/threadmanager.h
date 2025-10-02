#pragma once

#include <deque>
#include <thread>

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

  static void join(std::thread &thread);
};
