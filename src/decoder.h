#pragma once

#include <vector>
#include <string_view>

#include "event.h"

template <typename Consumer>
class Decoder : public Consumer {
public:
    Decoder(Consumer consumer)
        : Consumer(consumer)
    {}

    ~Decoder() {}

    void consumeData(const char *data, std::size_t size) {
        while (size != 0) {
            if (remainingSize != 0) {
                if (remainingSize <= size) {
                    if (catMsgs.empty()) {
                        static_cast<Consumer *>(this)->consumeEvent(Event(Instant::fromUint64(timeRegister), data, remainingSize));
                    } else {
                        char *dst = new char[sizeRegister];
                        std::size_t index = 0;
                        for (std::string_view sv : catMsgs) {
                            std::copy_n(sv.data(), sv.size(), dst + index);
                            index += sv.size();
                        }
                        catMsgs.clear();
                        assert(index + remainingSize == sizeRegister);
                        std::copy_n(data, remainingSize, dst + index);
                        static_cast<Consumer *>(this)->consumeEvent(Event(Instant::fromUint64(timeRegister), dst, sizeRegister));
                        static_cast<Consumer *>(this)->consumeBestow(dst);
                    }
                    data += remainingSize;
                    size -= remainingSize;
                } else {
                    catMsgs.emplace_back(data, size);
                    remainingSize -= size;
                    break;
                }
            } else if (metaState != 0) {
                if (metaState >> 4) {
                    metaState -= 0x10;
                    std::int64_t inc = static_cast<signed char>(*data);
                    inc *= 1ull << ((metaState >> 4) * 8);
                    timeRegister += inc;
                } else {
                    metaState -= 0x01;
                    std::int64_t inc = static_cast<signed char>(*data);
                    inc *= 1ull << (metaState * 8);
                    sizeRegister += inc;

                    if (metaState == 0) {
                        remainingSize = sizeRegister;
                    }
                }
                data++;
                size--;
            } else {
                metaState = *data & 0x77;
                data++;
                size--;
            }
        }
    }

    void consumeBestow(const char *data) {
        static_cast<Consumer *>(this)->consumeBestow(data);
    }

private:
    std::uint64_t timeRegister = 0;
    std::uint64_t sizeRegister = 0;
    char metaState = 0;
    std::size_t remainingSize = 0;
    std::vector<std::string_view> catMsgs;
};
