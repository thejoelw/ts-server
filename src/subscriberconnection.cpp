#include "subscriberconnection.h"

#include "stream.h"
#include "wsconn.h"
#include "subconnmanager.h"

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

void SubscriberConnection::emit(Event event) {
    if (event.time >= beginTime) {
        if (event.time < endTime) {
            wsConn->send(std::to_string(event.time.toUint64()) + " " + std::string(event.data, event.size), uWS::OpCode::TEXT, true);
            if (--head == 0) {
                endTime = Instant::fromUint64(0);
                dispatchClose();
            }
        } else {
            dispatchClose();
        }
    }
}

void SubscriberConnection::dispatchClose() {
    SubConnManager::getInstance().dispatchClose(this);
}
