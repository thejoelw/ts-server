#include "subscriberconnection.h"

#include "stream.h"
#include "subconnmanager.h"

SubscriberConnection::SubscriberConnection()
    : spec(SubSpec{ .beginTime = Instant::now(), .endTime = Instant::now(), .head = 0, .tail = 0 })
    , nextChunkId(static_cast<std::size_t>(-1))
    , nextEventId(static_cast<std::size_t>(-1))
{}

SubscriberConnection::SubscriberConnection(Stream *stream, SubSpec spec)
    : stream(stream)
    , spec(spec)
    , nextChunkId(stream->getInitChunkId(spec.beginTime))
    , nextEventId(nextChunkId < stream->getNumChunks() ? stream->getChunk(nextChunkId).getInitEventId(spec.beginTime) : 0)
{}

void SubscriberConnection::tick() {
    wsConn->cork([this]() {
        try {
            stream->tick(*this);
        } catch (const BackoffException &ex) {}
    });
}

SubWsConn::SendStatus SubscriberConnection::emit(Event event) {
    if (event.time >= spec.beginTime) {
        if (event.time < spec.endTime) {
            SubWsConn::SendStatus status = wsConn->send(std::string_view(event.data, event.size), uWS::OpCode::BINARY, true);
            if (--spec.head == 0) {
                spec.endTime = Instant::fromUint64(0);
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
