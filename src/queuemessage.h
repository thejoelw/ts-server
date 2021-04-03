#pragma once

#include <variant>
#include <memory>
#include <vector>

#include "uWebSockets/src/MoveOnlyFunction.h"

#include "event.h"

struct Commit {
    Commit(uWS::MoveOnlyFunction<void()> &&onFlush)
        : onFlush(std::move(onFlush))
    {}

    uWS::MoveOnlyFunction<void()> onFlush;
};

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
    Commit,
    Bestow<std::unique_ptr<char[]>>,
    Bestow<std::shared_ptr<char>>,
    Bestow<std::vector<char>>,
    Yield,
    Join
> QueueMessage;
