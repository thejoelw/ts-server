#pragma once

#include <assert.h>
#include <cstdlib>
#include <iostream>

#include <zstd.h>

template <typename Consumer>
class Decompressor : public Consumer {
public:
    template <typename... ConsumerArgs>
    Decompressor(ConsumerArgs... args)
        : Consumer(std::forward<ConsumerArgs>(args)...)
        , context(ZSTD_createDCtx())
        , outBuf { 0, ZSTD_DStreamOutSize(), ZSTD_DStreamOutSize() }
    {}

    ~Decompressor() {
        mem = static_cast<Consumer *>(this)->onBestow(std::move(mem));

        if (!ended) {
            static_cast<Consumer *>(this)->onError("ZSTD frame is not ended!");
        }

        ZSTD_freeDCtx(context);
    }

    std::size_t getPrefferedSize() const {
        return ZSTD_DStreamInSize();
    }

    void onData(const char *data, std::size_t size) {
        if (ended) {
            static_cast<Consumer *>(this)->onError("More data after ZSTD frame ended!");
            return;
        }

        ZSTD_inBuffer inBuf = { data, size, 0 };
        while (inBuf.pos < inBuf.size) {
            prepareOutBuf();
            std::size_t prevPos = outBuf.pos;
            std::size_t res = ZSTD_decompressStream(context, &outBuf, &inBuf);
            
            if (ZSTD_isError(res)) {
                static_cast<Consumer *>(this)->onError(std::string("Zstd error: ") + ZSTD_getErrorName(res));
                ended = true;
                break;
            }

            if (outBuf.pos != prevPos) {
                static_cast<Consumer *>(this)->onData(static_cast<char *>(outBuf.dst) + prevPos, outBuf.pos - prevPos);
            }

            ended = res == 0;
        }
    }

    template <typename MemType>
    MemType onBestow(MemType mem) {
        // Don't need the memory any more (already consumed by zstd), so can immediately give back to producer for re-use.
        return std::forward<MemType>(mem);
    }

private:
    ZSTD_DCtx* context;
    std::unique_ptr<char[]> mem;
    ZSTD_outBuffer outBuf;
    bool ended = false;

    void prepareOutBuf() {
        if (outBuf.pos == outBuf.size) {
            mem = static_cast<Consumer *>(this)->onBestow(std::move(mem));
            if (!mem) {
                mem = std::make_unique<char[]>(outBuf.size);
            }
            outBuf.dst = mem.get();
            assert(outBuf.size == ZSTD_DStreamOutSize());
            outBuf.pos = 0;
        }
    }
};
