#pragma once

#include <assert.h>
#include <cstdlib>
#include <iostream>

#include <zstd.h>

#include "dbparseexception.h"

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

        if (!frameEnded) {
            DbParseException::getStore().emplace_back("ZSTD frame is not ended!");
        }
    }

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
            frameEnded = res == 0;
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
    bool frameEnded = true;

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
            static thread_local unsigned int remaining = 10;
            if (remaining) {
                std::cerr << "Zstd error: " << ZSTD_getErrorName(code) << std::endl;
                if (--remaining == 0) {
                    std::cerr << "Zstd errors truncated" << std::endl;
                }
            }
        }
    }
};
