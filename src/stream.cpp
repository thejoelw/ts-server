#include "stream.h"

#include "subscriberconnection.h"

Stream::Stream(const std::string &key)
    : key(key)
{

}

std::uint32_t Stream::getInitChunkId(Instant beginTime) {
    return std::lower_bound(chunks.cbegin(), chunks.cend(), beginTime) - chunks.cbegin();
}

void Stream::tick(SubscriberConnection &conn) {
    // TODO: Handle tail here - go backwards from end instead of forwards

    if (conn.nextChunkId < chunks.size()) {
        chunks[conn.nextChunkId].tick(conn);
    } else {
        if (Instant::now() >= conn.endTime) {
            conn.nextChunkId = static_cast<std::uint32_t>(-1);
            throw UnsubscribeException();
        }
        if (conn.nextChunkId != static_cast<std::uint32_t>(-1)) {
            conn.nextChunkId = static_cast<std::uint32_t>(-1);
            conn.wsConn->subscribe(getKey());
        }
    }
}

void Stream::publish(Event event) {
    if (chunks.back().getNumEvents() >= 1024 * 1024) {
        chunks.emplace_back(this, event.time);
        writerManager.open(chunks.back().getFilename());
    }
    // TODO: Copy event into chunk and bestow memory
    // Need to bestow to both of these places (beacuse either may outlive the other):
    chunks.back().recvEvent(event);
    writerManager.write(event);
}
