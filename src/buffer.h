#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "uWebSockets/src/MoveOnlyFunction.h"

template <typename Consumer, std::size_t bufSize> class Buffer : public Consumer {
public:
  template <typename... ConsumerArgs>
  Buffer(ConsumerArgs... args)
      : Consumer(std::forward<ConsumerArgs>(args)...)
      , bufMem(std::make_unique<char[]>(bufSize)) {}

  ~Buffer() { flush(); }

  void onData(const char *data, std::size_t size) {
    if (bufIdx + size > bufSize) {
      flush();

      if (size >= bufSize) {
        static_cast<Consumer *>(this)->onData(data, size);
        return;
      }
    }
    std::copy_n(data, size, bufMem.get() + bufIdx);
    bufIdx += size;
  }

  template <typename MemType> MemType onBestow(MemType mem) { return std::forward<MemType>(mem); }

  template <typename AckType> void onFlush(AckType ack) {
    flush();
    static_cast<Consumer *>(this)->onFlush(std::forward<AckType>(ack));
  }

private:
  std::unique_ptr<char[]> bufMem;
  std::size_t bufIdx = 0;

  void flush() {
    static_cast<Consumer *>(this)->onData(bufMem.get(), bufIdx);
    bufMem = static_cast<Consumer *>(this)->onBestow(std::move(bufMem));
    if (!bufMem) {
      bufMem = std::make_unique<char[]>(bufSize);
    }
    bufIdx = 0;
  }
};
