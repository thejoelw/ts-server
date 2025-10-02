#include "chunk.h"

#include "garbagecollector.h"
#include "options.h"
#include "readermanager.h"
#include "stream.h"
#include "subscriberconnection.h"

std::string Chunk::getFilename() const {
  return Options::getOptions().dataPath + "/chunk_" + stream->getKey() + "_" +
         std::to_string(beginTime.toUint64()) + ".bin";
}

std::pair<bool, std::uint64_t>
Chunk::parseFilename(const std::string &filename, const std::string &key) {
  std::string begin = Options::getOptions().dataPath + "/chunk_" + key + "_";
  std::string end = ".bin";
  if (filename.substr(0, begin.size()) == begin &&
      filename.substr(filename.size() - end.size()) == end) {
    return std::pair<bool, std::uint64_t>(
        true,
        std::stoull(filename.substr(begin.size(), filename.size() - begin.size() - end.size()))
    );
  } else {
    return std::pair<bool, std::uint64_t>(false, 0);
  }
}

std::size_t Chunk::getInitEventId(Instant beginTime) {
  std::vector<Event>::const_iterator found =
      std::lower_bound(events.cbegin(), events.cend(), beginTime, [](Event t0, Instant t1) {
        return t0.time < t1;
      });
  return found - events.cbegin();
}

void Chunk::gc() {
  assert(status == Status::Done);
  *this = Chunk(stream, beginTime, true);
  assert(events.size() == 0);
  assert(events.capacity() == 0);
  assert(status == Status::Closed);
}

void Chunk::onEvent(Event event) {
  assert(status == Status::Reading || status == Status::Live);
  assert(events.empty() || event.time >= events.back().time);
  events.push_back(event);
}

void Chunk::onEnd() {
  assert(status == Status::Reading || status == Status::Live);
  status = Status::Done;
  GarbageCollector::getInstance().submitDone(this);
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
    if (emitEvents(conn)) {
      conn.nextChunkId++;
      conn.nextEventId = 0;
    }
    break;

  case Status::Live:
    if (emitEvents(conn)) {
      stream->addRealtimeSub(&conn);
    }
    break;
  }
}

bool Chunk::emitEvents(SubscriberConnection &conn) {
  while (conn.nextEventId < events.size()) {
    SubWsConn::SendStatus status = conn.emit(events[conn.nextEventId]);
    switch (status) {
    case SubWsConn::SendStatus::SUCCESS:
      conn.nextEventId++;
      break;
    case SubWsConn::SendStatus::BACKPRESSURE:
      conn.nextEventId++;
      return false;
    case SubWsConn::SendStatus::DROPPED:
      return false;
    }
  }

  return true;
}
