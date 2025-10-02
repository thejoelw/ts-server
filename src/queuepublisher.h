#pragma once

#include "readerwriterqueue/readerwriterqueue.h"

#include "queuemessage.h"

class QueuePublisher {
public:
  QueuePublisher(moodycamel::ReaderWriterQueue<QueueMessage> &queue)
      : queue(queue) {}

  ~QueuePublisher() {
    for (unsigned int i = 0; i < 16; i++) {
      queue.enqueue(Yield());
    }
    queue.enqueue(Join());
  }

  void onEvent(Event event) { queue.enqueue(event); }

  template <typename MemType> MemType onBestow(MemType mem) {
    queue.enqueue(Bestow(std::forward<MemType>(mem)));
    // We moved the memory to the queue, so it's "gone"
    return MemType();
  }

  void onError(std::string_view msg) { queue.enqueue(Error(msg)); }

private:
  moodycamel::ReaderWriterQueue<QueueMessage> &queue;
};
