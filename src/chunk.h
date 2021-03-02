#pragma once

#include <string>
#include <vector>

#include "instant.h"
#include "event.h"

class Stream;
class SubscriberConnection;

class Chunk {
private:
    enum class Status {Lazy, Reading, Done};

public:
    Chunk(Stream *stream, Instant beginTime)
        : stream(stream)
        , beginTime(beginTime)
    {}

    ~Chunk() {
        freeMemory();
    }

    bool operator<(const Chunk &other) const {
        return beginTime < other.beginTime;
    }
    bool operator<(Instant otherTime) const {
        return beginTime < otherTime;
    }

    std::string getFilename() const;

    std::size_t getNumEvents() const { return events.size(); }

    void gc();

    void recvEvent(Event event);
    void recvBestow(const char *data);
    void recvEnd();

    void tick(SubscriberConnection &conn);

private:
    Stream *stream;

    Instant beginTime;

    std::string data;
    std::vector<Event> events;
    std::vector<const char *> bestowed;
    Status status = Status::Lazy;

    void emitEvents(SubscriberConnection &conn);
    void freeMemory();
};
