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
        {
            shouldRun.test_and_set();
            thread = std::thread(func, this);
        }

        Chunk *chunk;
        moodycamel::ReaderWriterQueue<QueueMessage> queue;
        std::thread thread;
        std::atomic_flag shouldRun = ATOMIC_FLAG_INIT;
    };

public:
    static ReaderManager &getInstance() {
        static ReaderManager inst;
        return inst;
    }

    ~ReaderManager();

    void addReader(Chunk *chunk);

    void tick();

    void joinAll();

private:
    std::deque<Reader> readers;
};
