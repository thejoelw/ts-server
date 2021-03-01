#pragma once

#include "readerwriterqueue/readerwriterqueue.h"

#include <thread>
#include <deque>

#include "queuemessage.h"

class Chunk;

class ReaderManager {
private:
    struct Reader {
        template <typename Func>
        Reader(Chunk *chunk, Func func)
            : chunk(chunk)
            , thread(func, this)
        {}

        Chunk *chunk;
        moodycamel::ReaderWriterQueue<QueueMessage> queue;
        std::thread thread;

        std::size_t remaining;


    };

public:
    static ReaderManager &getInstance() {
        static ReaderManager inst;
        return inst;
    }

    void addReader(Chunk *chunk);

    void tick();

private:
    std::deque<Reader> readers;
};
