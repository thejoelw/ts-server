#pragma once

#include <string>

#include "instant.h"

class MessageFormatter {
public:
  MessageFormatter(bool prefixTimestamp);

  std::string_view format(std::string_view data, Instant timestamp);

private:
  bool prefixTimestamp;

  std::string buf;
};