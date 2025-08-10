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
{
    if (!spec.jqQuery.empty()) {
        jqProcessor = std::make_unique<JqProcessor>(spec.jqQuery);
    }
}

void SubscriberConnection::tick() {
    wsConn->cork([this]() {
        stream->tick(*this);
    });
}

bool SubscriberConnection::emit(Event event) {
    if (event.time < spec.beginTime) {
        return false;
    }

    if (event.time >= spec.endTime) {
        dispatchClose();
        return false;
    }

    SubWsConn::SendStatus status;

    if (!emitQueue.empty()) {
        std::vector<std::string>::const_iterator it = emitQueue.cbegin();
        while (it != emitQueue.cend()) {
            SubWsConn::SendStatus status = wsConn->send(*it, uWS::OpCode::BINARY, true);
            if (status == SubWsConn::SendStatus::BACKPRESSURE) {
                status = SubWsConn::SendStatus::BACKPRESSURE;
            } else if (status == SubWsConn::SendStatus::DROPPED) {
                status = SubWsConn::SendStatus::DROPPED;
                break;
            }
        }
        emitQueue.erase(emitQueue.begin(), it);
    }

    std::string_view eventStr(event.data, event.size);

    // Apply JQ filter if configured
    if (jqProcessor) {
        status = SubWsConn::SendStatus::SUCCESS;
        jqProcessor->process(eventStr, [this, &status](std::string_view data) {
            if (status == SubWsConn::SendStatus::DROPPED) {
                emitQueue.emplace_back(data);
                return;
            }

            SubWsConn::SendStatus curStatus = wsConn->send(data, uWS::OpCode::BINARY, true);
            if (curStatus == SubWsConn::SendStatus::BACKPRESSURE) {
                status = SubWsConn::SendStatus::BACKPRESSURE;
            } else if (curStatus == SubWsConn::SendStatus::DROPPED) {
                status = SubWsConn::SendStatus::DROPPED;
                emitQueue.emplace_back(data);
            }
        });
    } else {
        status = wsConn->send(eventStr, uWS::OpCode::BINARY, true);
    }

    if (--spec.head == 0) {
        spec.endTime = Instant::fromUint64(0);
        dispatchClose();
    }

    return status != SubWsConn::SendStatus::SUCCESS;
}

void SubscriberConnection::dispatchClose() {
    SubConnManager::getInstance().dispatchClose(this);
}
