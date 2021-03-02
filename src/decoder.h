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
            } else if (metaCtl != 0) {
                if (metaCtl >> 4) {
                    metaCtl -= 0x10;
                    timeInc <<= 8;
                    timeInc += static_cast<unsigned char>(*data);
                } else {
                    metaCtl -= 0x01;
                    sizeInc <<= 8;
                    sizeInc += static_cast<unsigned char>(*data);

                    if (metaCtl == 0) {
                        // ZigZag decoding
                        timeRegister += (timeInc >> 1) ^ -(timeInc & 1);
                        sizeRegister += (sizeInc >> 1) ^ -(sizeInc & 1);
                        remainingSize = sizeRegister;
                    }
                }
                data++;
                size--;
            } else {
                metaCtl = *data;
                timeInc = 0;
                sizeInc = 0;
                remainingSize = sizeRegister; // In case metaState == 0
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
    char metaCtl = 0;
    std::uint64_t timeInc;
    std::uint64_t sizeInc;
    std::size_t remainingSize = 0;
    std::vector<std::string_view> catMsgs;
};
