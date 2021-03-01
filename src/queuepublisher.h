#pragma once

#include "readerwriterqueue/readerwriterqueue.h"

#include "queuemessage.h"

class QueuePublisher {
public:
    QueuePublisher(moodycamel::ReaderWriterQueue<QueueMessage> &queue)
        : queue(queue)
    {}

    ~QueuePublisher() {
        for (unsigned int i = 0; i < 256; i++) {
            queue.enqueue(Yield());
        }
        queue.enqueue(Join());
    }

    void consumeEvent(Event event) {
        queue.enqueue(event);
    }

    void consumeBestow(const char *data) {
        queue.enqueue(Bestow(data));
    }

private:
    moodycamel::ReaderWriterQueue<QueueMessage> &queue;
};
