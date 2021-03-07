#include "chunk.h"

#include "subscriberconnection.h"
#include "readermanager.h"
#include "stream.h"
#include "options.h"

std::string Chunk::getFilename() const {
    return Options::getOptions().dataPath + "/chunk_" + stream->getKey() + "_" + std::to_string(beginTime.toUint64()) + ".bin";
}

std::pair<bool, std::uint64_t> Chunk::parseFilename(const std::string &filename, const std::string &key) {
    std::string begin = Options::getOptions().dataPath + "/chunk_" + key + "_";
    std::string end = ".bin";
    if (filename.substr(0, begin.size()) == begin && filename.substr(filename.size() - end.size()) == end) {
        return std::pair<bool, std::uint64_t>(true, std::stoull(filename.substr(begin.size(), filename.size() - begin.size() - end.size())));
    } else {
        return std::pair<bool, std::uint64_t>(false, 0);
    }
}

std::size_t Chunk::getInitEventId(Instant beginTime) {
    std::vector<Event>::const_iterator found = std::lower_bound(events.cbegin(), events.cend(), beginTime, [](Event t0, Instant t1) { return t0.time < t1; });
    return found - events.cbegin();
}

void Chunk::gc() {}

void Chunk::onEvent(Event event) {
    assert(events.empty() || event.time >= events.back().time);
    events.push_back(event);
}

void Chunk::onEnd() {
    assert(status == Status::Reading);
    status = Status::Done;
}

void Chunk::tick(SubscriberConnection &conn) {
    switch (status) {
    case Status::Closed:
        // Need to initialize the loading
        ReaderManager::getInstance().addReader(this);
        status = Status::Reading;
        break;

    case Status::Reading:
        emitEvents(conn);
        break;

    case Status::Done:
    case Status::Live:
        emitEvents(conn);
        conn.nextChunkId++;
        conn.nextEventId = 0;
        break;
    }
}

void Chunk::emitEvents(SubscriberConnection &conn) {
    while (conn.nextEventId < events.size()) {
        conn.emit(events[conn.nextEventId++]);
    }
}
