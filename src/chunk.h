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

    bool operator<(const Chunk &other) const {
        return beginTime < other.beginTime;
    }
    bool operator<(Instant otherTime) const {
        return beginTime < otherTime;
    }

    std::string getFilename() const;
    static std::pair<bool, std::uint64_t> parseFilename(const std::string &filename, const std::string &key);

    std::size_t getNumEvents() const { return events.size(); }

    void gc();

    void onEvent(Event event);
    void onEnd();

    template <typename MemType>
    MemType onBestow(MemType mem) {
        std::get<std::vector<MemType>>(bestowed).push_back(std::forward<MemType>(mem));
        return MemType();
    }

    void tick(SubscriberConnection &conn);

private:
    Stream *stream;

    Instant beginTime;

    std::string data;
    std::vector<Event> events;
    Status status = Status::Lazy;

    std::tuple<
        std::vector<std::unique_ptr<char[]>>,
        std::vector<std::shared_ptr<char>>,
        std::vector<std::vector<char>>
    > bestowed;

    void emitEvents(SubscriberConnection &conn);
};
