#pragma once

#include <string_view>

#include "wsconn.h"
#include "instant.h"
#include "event.h"
#include "baseexception.h"

class Stream;

class SubscriberConnection {
public:
    SubscriberConnection();
    SubscriberConnection(Stream *topic, Instant beginTime, Instant endTime, std::uint64_t head, std::uint64_t tail);

    WsConn *wsConn = 0;

    Stream *stream = 0;
    std::size_t nextChunkId;
    std::size_t nextEventId;

    Instant beginTime;
    Instant endTime;
    std::uint64_t head;
    std::uint64_t tail;

    void tick();

    void emit(Event event);

    void dispatchClose();
};
