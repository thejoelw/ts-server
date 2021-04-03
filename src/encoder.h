#pragma once

#include <vector>
#include <string_view>

#include "event.h"

template <typename Consumer>
class Encoder : public Consumer {
public:
    template <typename... ConsumerArgs>
    Encoder(ConsumerArgs... args)
        : Consumer(std::forward<ConsumerArgs>(args)...)
    {}

    ~Encoder() {
        onEvent(Event(Instant::fromUint64(0), 0, 0));
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

        char metaArr[17];
        char *metaDst = metaArr;
        *metaDst++ = metaCtl;
        for (unsigned int i = timeBytes; i-- > 0;) {
            *metaDst++ = (dt >> (i * 8)) & 0xFF;
        }
        for (unsigned int i = sizeBytes; i-- > 0;) {
            *metaDst++ = (ds >> (i * 8)) & 0xFF;
        }

        static_cast<Consumer *>(this)->onData(metaArr, 1 + timeBytes + sizeBytes);
        static_cast<Consumer *>(this)->onData(event.data, event.size);

        timeRegister = time;
        sizeRegister = size;
    }

    template <typename MemType>
    MemType onBestow(MemType mem) {
        // Forward memory to be managed by the consumer, since we've forwarded some char bufs.
        return static_cast<Consumer *>(this)->onBestow(std::forward<MemType>(mem));
    }

    template <typename AckType>
    void onFlush(AckType ack) {
        static_cast<Consumer *>(this)->onFlush(std::forward<AckType>(ack));
    }

private:
    std::uint64_t timeRegister = 0;
    std::uint64_t sizeRegister = 0;
};
