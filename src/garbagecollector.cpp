#include "garbagecollector.h"

#include <assert.h>

#include "chunk.h"

static constexpr unsigned int maxDoneChunks = MAX_LOADED_CHUNKS;

void GarbageCollector::submitDone(Chunk *chunk) {
    assert(chunk->getStatus() == Chunk::Status::Done);
    chunks.push_back(chunk);
    if (chunks.size() > maxDoneChunks) {
        chunks.front()->gc();
        chunks.pop_front();
    }
}
