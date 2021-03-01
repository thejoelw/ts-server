#pragma once

#include <string>
#include <vector>

#include "instant.h"
#include "event.h"

class Connection;

class Chunk {
private:
    enum class Status {Lazy, Reading, Done};

public:
    Chunk(Instant beginTime, Instant endTime)
        : beginTime(beginTime)
        , endTime(endTime)
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

    void gc();

    void recvEvent(Event event);
    void recvBestow(const char *data);

    void tick(Connection &conn);

private:
    Instant beginTime;
    Instant endTime;

    std::string data;
    std::vector<Event> events;
    std::vector<const char *> bestowed;
    Status status = Status::Lazy;

    void emitEvents(Connection &conn);
    void freeMemory();
};
