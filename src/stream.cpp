#include "stream.h"

#include "connection.h"

std::uint32_t Stream::getInitChunkId(Instant beginTime) {
    return std::lower_bound(chunks.cbegin(), chunks.cend(), beginTime) - chunks.cbegin();
}

void Stream::tick(Connection &conn) {
    if (conn.nextChunkId < chunks.size()) {
        chunks[conn.nextChunkId].tick(conn);
    } else if (conn.endTime < Instant::now()) {
        throw UnsubscribeException();
    }
}
