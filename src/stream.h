#pragma once

#include <deque>

#include "instant.h"
#include "chunk.h"
#include "baseexception.h"

class Connection;

class Stream {
public:
    class UnsubscribeException : public BaseException {
    public:
        UnsubscribeException()
            : BaseException("unsubscribe")
        {}
    };

    std::uint32_t getInitChunkId(Instant beginTime);

    void tick(Connection &conn);

private:
    std::deque<Chunk> chunks;
};
