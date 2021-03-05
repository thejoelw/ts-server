#pragma once

#include <variant>
#include <memory>
#include <vector>

#include "event.h"

template <typename MemType>
struct Bestow {
    Bestow(MemType &&mem)
        : mem(std::move(mem))
    {}

    MemType mem;
};
struct Yield {};
struct Join {};

typedef std::variant<
    std::monostate,
    Event,
    Bestow<std::unique_ptr<char[]>>,
    Bestow<std::shared_ptr<char>>,
    Bestow<std::vector<char>>,
    Yield,
    Join
> QueueMessage;
