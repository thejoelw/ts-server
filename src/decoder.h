#pragma once

#include <vector>
#include <string_view>

#include "event.h"
#include "dbparseexception.h"

template <typename Consumer>
class Decoder : public Consumer {
public:
    template <typename... ConsumerArgs>
    Decoder(ConsumerArgs... args)
        : Consumer(std::forward<ConsumerArgs>(args)...)
    {}

    ~Decoder() {
        if (timeRegister != 0) {
            DbParseException::getStore().emplace_back("Time register is not zero");
        }
        if (sizeRegister != 0) {
            DbParseException::getStore().emplace_back("Size register is not zero");
        }
        if (metaCtl != 0) {
            DbParseException::getStore().emplace_back("Meta ctl is not zero");
        }
        if (remainingSize != 0) {
            DbParseException::getStore().emplace_back("Remaining size is not zero");
        }
        if (!catMsgs.empty()) {
            DbParseException::getStore().emplace_back("Cat msgs is not empty");
        }
    }

    void onData(const char *data, std::size_t size) {
        while (size != 0) {
            if (remainingSize != 0) {
                if (remainingSize <= size) {
                    if (catMsgs.empty()) {
                        static_cast<Consumer *>(this)->onEvent(Event(Instant::fromUint64(timeRegister), data, remainingSize));
                    } else {
                        catMsgs.insert(catMsgs.cend(), data, data + remainingSize);
                        assert(catMsgs.size() == sizeRegister);
                        static_cast<Consumer *>(this)->onEvent(Event(Instant::fromUint64(timeRegister), catMsgs.data(), catMsgs.size()));
                        catMsgs = static_cast<Consumer *>(this)->onBestow(std::move(catMsgs));
                        catMsgs.clear();
                    }
                    data += remainingSize;
                    size -= remainingSize;
                    remainingSize = 0;
                } else {
                    catMsgs.insert(catMsgs.cend(), data, data + size);
                    remainingSize -= size;
                    size = 0;
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
                }
                if (metaCtl == 0) {
                    // ZigZag decoding
                    timeRegister += (timeInc >> 1) ^ -(timeInc & 1);
                    sizeRegister += (sizeInc >> 1) ^ -(sizeInc & 1);
                    remainingSize = sizeRegister;
                }
                data++;
                size--;
            } else {
                metaCtl = *data;
                timeInc = 0;
                sizeInc = 0;
                remainingSize = metaCtl ? 0 : sizeRegister;
                data++;
                size--;
            }
        }
    }

    template <typename MemType>
    MemType onBestow(MemType mem) {
        // Forward memory to be managed by the consumer, since we've forwarded some char bufs.
        return static_cast<Consumer *>(this)->onBestow(std::forward<MemType>(mem));
    }

private:
    std::uint64_t timeRegister = 0;
    std::uint64_t sizeRegister = 0;
    char metaCtl = 0;
    std::uint64_t timeInc;
    std::uint64_t sizeInc;
    std::size_t remainingSize = 0;
    std::vector<char> catMsgs;
};
