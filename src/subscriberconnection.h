#pragma once

#include <string_view>

#include "event.h"
#include "subspec.h"
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
    SubscriberConnection(Stream *topic, SubSpec spec);

    SubWsConn *wsConn = 0;

    Stream *stream = 0;
    SubSpec spec;

    std::size_t nextChunkId;
    std::size_t nextEventId;

    void tick();

    SubWsConn::SendStatus emit(Event event);

    void dispatchClose();
};
