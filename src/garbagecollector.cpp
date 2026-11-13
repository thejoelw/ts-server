#include "garbagecollector.h"

#include <assert.h>

#include "chunk.h"
#include "stream.h"
#include "subconnmanager.h"
#include "subscriberconnection.h"

static constexpr unsigned int maxDoneChunks = MAX_LOADED_CHUNKS;

// A chunk is pinned while a subscriber is still positioned in it. Collecting it would reset it to
// Closed and make the next tick re-read and re-decompress the whole file from disk. That is still
// correct - the re-read yields the same events in the same order, so the subscriber's event index
// stays valid - but with a small maxDoneChunks two subscribers at different offsets would evict
// each other's chunk on every completion and never make progress.
static bool isPinned(const Chunk *chunk) {
  for (SubscriberConnection *conn : SubConnManager::getInstance().getConnections()) {
    if (!conn->stream || conn->nextChunkId >= conn->stream->getNumChunks()) {
      // Either not attached to a stream yet, or a realtime subscriber (nextChunkId is -1), which
      // reads events straight off the publish path and holds no chunk.
      continue;
    }
    if (&conn->stream->getChunk(conn->nextChunkId) == chunk) {
      return true;
    }
  }
  return false;
}

void GarbageCollector::submitDone(Chunk *chunk) {
  assert(chunk->getStatus() == Chunk::Status::Done);
  chunks.push_back(chunk);

  // Collect from the oldest end, skipping pinned chunks rather than stopping at them, so a
  // long-lived subscriber parked on an old chunk doesn't block collection of everything behind it.
  // If every candidate is pinned we deliberately stay over the limit; exceeding it costs memory,
  // evicting a chunk out from under its reader costs a 200MB+ re-read.
  std::deque<Chunk *>::iterator it = chunks.begin();
  while (chunks.size() > maxDoneChunks && it != chunks.end()) {
    if (isPinned(*it)) {
      it++;
      continue;
    }
    (*it)->gc();
    it = chunks.erase(it);
  }
}
