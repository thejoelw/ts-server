#pragma once

#include "instant.h"
#include <string>

class SubSpec {
public:
  Instant beginTime = Instant::epoch();
  Instant endTime = Instant::epoch();

  std::uint64_t head = 0;
  std::uint64_t tail = 0;

  Instant replayStartTime = Instant::epoch();
  double replaySpeed = std::numeric_limits<double>::infinity();

  std::string jqQuery;
};
