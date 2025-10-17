#pragma once

#include <chrono>

typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> InstantBase;

class Instant : public InstantBase {
public:
  Instant(InstantBase base)
      : InstantBase(base) {}

  static Instant epoch() { return InstantBase(InstantBase::duration(0)); }

  static Instant now() {
    return Instant(
        std::chrono::time_point_cast<InstantBase::duration>(std::chrono::system_clock::now())
    );
  }

  static Instant fromUint64(std::uint64_t num) { return InstantBase(InstantBase::duration(num)); }

  std::uint64_t toUint64() const {
    return std::chrono::time_point_cast<InstantBase::duration>(*this).time_since_epoch().count();
  }
};
