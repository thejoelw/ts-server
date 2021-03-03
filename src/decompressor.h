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

    std::size_t getPrefferedSize() const {
        return ZSTD_DStreamInSize();
    }

    void onData(const char *data, std::size_t size) {
        ZSTD_inBuffer inBuf = { data, size, 0 };
        while (inBuf.pos < inBuf.size) {
            prepareOutBuf();
            std::size_t prevPos = outBuf.pos;
            std::size_t res = ZSTD_decompressStream(context, &outBuf, &inBuf);
            checkZstdRes(res);
            if (outBuf.pos != prevPos) {
                static_cast<Consumer *>(this)->onData(static_cast<char *>(outBuf.dst) + prevPos, outBuf.pos - prevPos);
            }
            if (res == 0) {
                // Frame ended, done
            }
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

    void checkZstdRes(std::size_t code) {
        if (ZSTD_isError(code)) {
            std::cerr << "ZSTD ERROR: " << ZSTD_getErrorName(code) << std::endl;
        }
    }
};
