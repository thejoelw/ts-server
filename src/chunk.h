#pragma once

#include <string>
#include <vector>

#include "instant.h"
#include "event.h"

class Stream;
class SubscriberConnection;

class Chunk {
public:
    Chunk(Stream *stream, Instant beginTime, bool fromFile)
        : stream(stream)
        , beginTime(beginTime)
        , status(fromFile ? Status::Closed : Status::Live)
    {}

    std::string getFilename() const;
    static std::pair<bool, std::uint64_t> parseFilename(const std::string &filename, const std::string &key);

    Instant getBeginTime() const { return beginTime; }

    enum class Status { Closed, Reading, Done, Live };
    Status getStatus() const { return status; }

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
    Status status = Status::Closed;

    std::tuple<
        std::vector<std::unique_ptr<char[]>>,
        std::vector<std::shared_ptr<char>>,
        std::vector<std::vector<char>>
    > bestowed;

    void emitEvents(SubscriberConnection &conn);
};
