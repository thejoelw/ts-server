#pragma once

#include <deque>

static constexpr unsigned int maxDoneChunks = 4;

class Chunk;

class GarbageCollector {
public:
    static GarbageCollector &getInstance() {
        static GarbageCollector inst;
        return inst;
    }

    void submitDone(Chunk *chunk);

    std::deque<Chunk *> chunks;
};
