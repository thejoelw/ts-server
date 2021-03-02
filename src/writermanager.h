#pragma once

#include "readerwriterqueue/readerwriterqueue.h"

#include <thread>

#include "queuemessage.h"

class WriterManager {
public:
    void open(const std::string &filename);

    void write(Event event);

private:
    moodycamel::ReaderWriterQueue<QueueMessage> *queue = 0;
    std::thread thread;
};
