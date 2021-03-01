#pragma once

#include <variant>

#include "event.h"

struct Bestow {
    Bestow(const char *data)
        : data(data)
    {}

    const char *data;
};
struct Yield {};
struct Join {};

typedef std::variant<std::monostate, Event, Bestow, Yield, Join> QueueMessage;
