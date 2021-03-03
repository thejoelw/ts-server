#pragma once

#include "readerwriterqueue/readerwriterqueue.h"

#include <thread>

#include "queuemessage.h"

class WriterManager {
public:
    ~WriterManager();

    void open(const std::string &filename);
    void close();

    void onEvent(Event event);

    template <typename MemType>
    MemType onBestow(MemType mem) {
        assert(queue);
        queue->enqueue(Bestow<MemType>(std::forward<MemType>(mem)));
        return MemType();
    }

private:
    moodycamel::ReaderWriterQueue<QueueMessage> *queue = 0;
    std::thread thread;
};
