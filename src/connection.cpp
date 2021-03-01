#include "connection.h"

#include "stream.h"
#include "wsconn.h"

Connection::Connection()
    : beginTime(Instant::now())
    , endTime(Instant::now())
{}

Connection::Connection(Stream *topic, Instant beginTime, Instant endTime)
    : topic(topic)
    , nextChunkId(topic->getInitChunkId(beginTime))
    , nextEventId(0)
    , beginTime(beginTime)
    , endTime(endTime)
{}

void Connection::tick() {
    topic->tick(*this);
}

void Connection::emit(std::string_view msg) {
    wsConn->send(msg, uWS::OpCode::BINARY, true);
}
