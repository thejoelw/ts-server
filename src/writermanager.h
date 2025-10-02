#pragma once

#include "readerwriterqueue/readerwriterqueue.h"

#include <thread>

#include "queuemessage.h"

class WriterManager {
public:
  ~WriterManager();

  bool isOpen() const {
    assert(!!queue == thread.joinable());
    return queue;
  }

  void open(const std::string &filename);
  void close();

  void onEvent(Event event);
  void onCommit(Commit commit);

  template <typename MemType> MemType onBestow(MemType mem) {
    if (isOpen()) {
      queue->enqueue(Bestow<MemType>(std::forward<MemType>(mem)));
    }
    return MemType();
  }

private:
  moodycamel::BlockingReaderWriterQueue<QueueMessage> *queue = 0;
  std::thread thread;
};
