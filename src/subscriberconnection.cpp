#include "subscriberconnection.h"

#include "stream.h"
#include "subconnmanager.h"

SubscriberConnection::SubscriberConnection()
    : beginTime(Instant::now())
    , endTime(Instant::now())
    , head(0)
    , tail(0)
{}

SubscriberConnection::SubscriberConnection(Stream *stream, Instant beginTime, Instant endTime, std::uint64_t head, std::uint64_t tail)
    : stream(stream)
    , nextChunkId(stream->getInitChunkId(beginTime))
    , nextEventId(nextChunkId < stream->getNumChunks() ? stream->getChunk(nextChunkId).getInitEventId(beginTime) : 0)
    , beginTime(beginTime)
    , endTime(endTime)
    , head(head)
    , tail(tail)
{}

void SubscriberConnection::tick() {
    try {
        stream->tick(*this);
    } catch (const BackoffException &ex) {}
}

SubWsConn::SendStatus SubscriberConnection::emit(Event event) {
    if (event.time >= beginTime) {
        if (event.time < endTime) {
            SubWsConn::SendStatus status = wsConn->send(std::string_view(event.data, event.size), uWS::OpCode::BINARY, true);
            if (--head == 0) {
                endTime = Instant::fromUint64(0);
                dispatchClose();
            }
            return status;
        } else {
            dispatchClose();
        }
    }
    return SubWsConn::SendStatus::SUCCESS;
}

void SubscriberConnection::dispatchClose() {
    SubConnManager::getInstance().dispatchClose(this);
}
