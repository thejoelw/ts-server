#pragma once

#include "instant.h"
#include <string>

class SubSpec {
public:
  Instant beginTime;
  Instant endTime;

  std::uint64_t head;
  std::uint64_t tail;

  std::string jqQuery;
};
