#pragma once

#include <deque>

class Chunk;

class GarbageCollector {
public:
  static GarbageCollector &getInstance() {
    static GarbageCollector inst;
    return inst;
  }

  void submitDone(Chunk *chunk);

  std::deque<Chunk *> chunks;
};
