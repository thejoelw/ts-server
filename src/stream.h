#pragma once

#include <deque>

#include "instant.h"
#include "chunk.h"
#include "event.h"
#include "baseexception.h"
#include "writermanager.h"

class SubscriberConnection;

class Stream {
public:
    class UnsubscribeException : public BaseException {
    public:
        UnsubscribeException()
            : BaseException("unsubscribe")
        {}
    };

    Stream(const std::string &key);

    const std::string &getKey() const { return key; }

    std::uint32_t getInitChunkId(Instant beginTime);

    void tick(SubscriberConnection &conn);

    void publish(Event event);

private:
    std::string key;

    std::deque<Chunk> chunks;

    WriterManager writerManager;
};
