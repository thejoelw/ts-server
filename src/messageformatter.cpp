#include "messageformatter.h"

static constexpr std::size_t initialSize = 1024;

MessageFormatter::MessageFormatter(bool prefixTimestamp)
    : prefixTimestamp(prefixTimestamp) {}

std::string_view MessageFormatter::format(std::string_view data, Instant timestamp) {
  if (prefixTimestamp) {
    buf.resize(initialSize);
    auto result = std::format_to_n(buf.data(), initialSize, "{}", timestamp.toUint64());
    buf.resize(result.out - buf.data());
    buf += ' ';
    buf += data;
    return buf;
  } else {
    return data;
  }
}