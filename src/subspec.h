#pragma once

#include "instant.h"
#include <string>

class SubSpec {
public:
  Instant beginTime = Instant::epoch();
  Instant endTime = Instant::epoch();

  std::uint64_t head = 0;
  std::uint64_t tail = 0;

  static constexpr std::chrono::microseconds disabledMinDelay = std::chrono::microseconds::min();
  std::chrono::microseconds minDelay = disabledMinDelay;
  bool printFirstEventTime = false;

  std::string jqQuery;
};
