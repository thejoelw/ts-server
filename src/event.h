#pragma once

#include "instant.h"

class Event {
public:
  Event(Instant time, const char *data, std::size_t size)
      : time(time)
      , data(data)
      , size(size) {}

  Instant time;
  const char *data;
  std::size_t size;
};
