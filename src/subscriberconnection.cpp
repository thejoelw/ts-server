#include "subscriberconnection.h"

#include "stream.h"
#include "wsconn.h"

SubscriberConnection::SubscriberConnection()
    : beginTime(Instant::now())
    , endTime(Instant::now())
    , head(0)
    , tail(0)
{}

SubscriberConnection::SubscriberConnection(Stream *topic, Instant beginTime, Instant endTime, std::uint64_t head, std::uint64_t tail)
    : stream(topic)
    , nextChunkId(topic->getInitChunkId(beginTime))
    , nextEventId(0)
    , beginTime(beginTime)
    , endTime(endTime)
    , head(head)
    , tail(tail)
{}

void SubscriberConnection::tick() {
    if (wsConn->getBufferedAmount() < 512 * 1024) {
        stream->tick(*this);
    }
}

void SubscriberConnection::emit(std::string_view msg) {
    wsConn->send(msg, uWS::OpCode::BINARY, true);
    if (--head == 0) {
        throw Stream::UnsubscribeException();
    }
}
