#pragma once

#include <string_view>
#include <vector>

#include "event.h"

template <typename Consumer> class Encoder : public Consumer {
public:
  template <typename... ConsumerArgs>
  Encoder(ConsumerArgs... args)
      : Consumer(std::forward<ConsumerArgs>(args)...) {}

  ~Encoder() {
    onEvent(Event(Instant::fromUint64(0), 0, 0));
    static_cast<Consumer *>(this)->onBestow(std::move(metaBufMem));
  }

  void onEvent(Event event) {
    assert((-1) >> 1 == -1);

    std::uint64_t time = event.time.toUint64();
    std::uint64_t size = event.size;

    std::uint64_t dt = time - timeRegister;
    std::uint64_t ds = size - sizeRegister;

    // ZigZag encoding
    dt = (dt << 1) ^ (static_cast<std::int64_t>(dt) >> 63);
    ds = (ds << 1) ^ (static_cast<std::int64_t>(ds) >> 63);

    unsigned int timeBytes = dt ? (64 + 7 - __builtin_clzll(dt)) / 8 : 0;
    unsigned int sizeBytes = ds ? (64 + 7 - __builtin_clzll(ds)) / 8 : 0;
    unsigned char metaCtl = (timeBytes << 4) | sizeBytes;

    unsigned int metaSize = 1 + timeBytes + sizeBytes;
    if (metaBufRemainingSize < metaSize) {
      bestowMeta();
      assert(metaBufRemainingSize >= metaSize);
    }
    metaBufRemainingSize -= metaSize;

    char *metaArr = metaBufMem.get() + metaBufRemainingSize;
    char *metaDst = metaArr;
    *metaDst++ = metaCtl;
    for (unsigned int i = timeBytes; i-- > 0;) {
      *metaDst++ = (dt >> (i * 8)) & 0xFF;
    }
    for (unsigned int i = sizeBytes; i-- > 0;) {
      *metaDst++ = (ds >> (i * 8)) & 0xFF;
    }

    static_cast<Consumer *>(this)->onData(metaArr, metaSize);
    static_cast<Consumer *>(this)->onData(event.data, event.size);

    timeRegister = time;
    sizeRegister = size;
  }

  template <typename MemType> MemType onBestow(MemType mem) {
    // Forward memory to be managed by the consumer, since we've forwarded some char bufs.
    return static_cast<Consumer *>(this)->onBestow(std::forward<MemType>(mem));
  }

  template <typename AckType> void onFlush(AckType ack) {
    static_cast<Consumer *>(this)->onFlush(std::forward<AckType>(ack));
  }

private:
  std::uint64_t timeRegister = 0;
  std::uint64_t sizeRegister = 0;

  static constexpr std::size_t metaBufSize = 1024 * 1024 - 16;
  std::unique_ptr<char[]> metaBufMem;
  std::size_t metaBufRemainingSize = 0;

  void bestowMeta() {
    metaBufMem = static_cast<Consumer *>(this)->onBestow(std::move(metaBufMem));
    if (!metaBufMem) {
      metaBufMem = std::make_unique<char[]>(metaBufSize);
    }
    metaBufRemainingSize = metaBufSize;
  }
};
