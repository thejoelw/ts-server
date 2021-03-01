#pragma once

#include <string_view>

#include "wsconn.h"
#include "instant.h"

class Stream;

class Connection {
public:
    Connection();
    Connection(Stream *topic, Instant beginTime, Instant endTime);

    WsConn *wsConn = 0;

    Stream *topic = 0;
    std::uint32_t nextChunkId;
    std::uint32_t nextEventId;

    Instant beginTime;
    Instant endTime;

    void tick();

    void emit(std::string_view msg);
};
