#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "uWebSockets/src/MoveOnlyFunction.h"

template <typename Consumer, std::size_t maxSize> class Chunker : public Consumer {
public:
  template <typename... ConsumerArgs>
  Chunker(ConsumerArgs... args)
      : Consumer(std::forward<ConsumerArgs>(args)...) {}

  ~Chunker() { flushData(); }

  void onData(const char *data, std::size_t size) {
    if (!datas.empty() && data == &datas.back().back()) {
      datas.back() = std::string_view(datas.back().data(), datas.back().size() + size);
    } else {
      datas.emplace_back(data, size);
    }

    curSize += size;
    if (curSize > maxSize) {
      flushData();
    }
  }

  template <typename MemType> MemType onBestow(MemType mem) {
    bestows.emplace_back([mem = std::forward<MemType>(mem)](Consumer *consumer) mutable {
      consumer->onBestow(std::forward<MemType>(mem));
    });
    return MemType();
  }

  template <typename AckType> void onFlush(AckType ack) {
    flushData();
    static_cast<Consumer *>(this)->onFlush(std::forward<AckType>(ack));
  }

private:
  std::vector<std::string_view> datas;
  std::vector<uWS::MoveOnlyFunction<void(Consumer *consumer)>> bestows;

  std::size_t curSize = 0;

  void flushData() {
    for (std::string_view data : datas) {
      static_cast<Consumer *>(this)->onData(data.data(), data.size());
    }
    datas.clear();

    for (uWS::MoveOnlyFunction<void(Consumer *consumer)> &bestow : bestows) {
      bestow(this);
    }
    bestows.clear();

    curSize = 0;
  }
};
