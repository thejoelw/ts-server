#pragma once

#include <string_view>

#include "instant.h"
#include "event.h"
#include "baseexception.h"

#include "uWebSockets/src/WebSocket.h"

class SubscriberConnection;

class Stream;

typedef uWS::WebSocket<false, true, SubscriberConnection> SubWsConn;

class SubscriberConnection {
public:
    class BackoffException : public BaseException {
    public:
        BackoffException()
            : BaseException("backoff")
        {}
    };

    SubscriberConnection();
    SubscriberConnection(Stream *topic, Instant beginTime, Instant endTime, std::uint64_t head, std::uint64_t tail);

    SubWsConn *wsConn = 0;

    Stream *stream = 0;
    std::size_t nextChunkId;
    std::size_t nextEventId;

    Instant beginTime;
    Instant endTime;
    std::uint64_t head;
    std::uint64_t tail;

    void tick();

    SubWsConn::SendStatus emit(Event event);

    void dispatchClose();
};
