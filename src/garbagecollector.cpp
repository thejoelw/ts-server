#include "garbagecollector.h"

#include "chunk.h"

void GarbageCollector::submitDone(Chunk *chunk) {
    assert(chunk->getStatus() == Chunk::Status::Done);
    chunks.push_back(chunk);
    if (chunks.size() > maxDoneChunks) {
        chunks.front()->gc();
        chunks.pop_front();
    }
}
