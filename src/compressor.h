#pragma once

#include <assert.h>
#include <cstdlib>
#include <iostream>

#include <zstd.h>

template <typename Consumer>
class Compressor : public Consumer {
public:
    template <typename... ConsumerArgs>
    Compressor(ConsumerArgs... args)
        : Consumer(std::forward<ConsumerArgs>(args)...)
        , context(ZSTD_createCCtx())
        , outBuf { 0, ZSTD_CStreamOutSize(), ZSTD_CStreamOutSize() }
    {
        checkZstdRes(ZSTD_CCtx_setParameter(context, ZSTD_c_compressionLevel, ZSTD_maxCLevel()));
        checkZstdRes(ZSTD_CCtx_setParameter(context, ZSTD_c_checksumFlag, 1));
    }

    ~Compressor() {
        ZSTD_inBuffer inBuf = { 0, 0, 0 };
        while (true) {
            prepareOutBuf();
            std::size_t prevPos = outBuf.pos;
            std::size_t res = ZSTD_compressStream2(context, &outBuf, &inBuf, ZSTD_e_end);
            checkZstdRes(res);
            if (outBuf.pos != prevPos) {
                static_cast<Consumer *>(this)->onData(static_cast<char *>(outBuf.dst) + prevPos, outBuf.pos - prevPos);
            }

            if (!res) {
                break;
            }
        }
    }

    std::size_t getPrefferedSize() const {
        return ZSTD_CStreamInSize();
    }

    void onData(const char *data, std::size_t size) {
        ZSTD_inBuffer inBuf = { data, size, 0 };
        do {
            prepareOutBuf();
            std::size_t prevPos = outBuf.pos;
            std::size_t res = ZSTD_compressStream2(context, &outBuf, &inBuf, ZSTD_e_continue);
            checkZstdRes(res);
            if (outBuf.pos != prevPos) {
                static_cast<Consumer *>(this)->onData(static_cast<char *>(outBuf.dst) + prevPos, outBuf.pos - prevPos);
            }
        } while (inBuf.pos != inBuf.size);
    }

    template <typename MemType>
    MemType onBestow(MemType mem) {
        // Don't need the memory any more (already consumed by zstd), so can immediately give back to producer for re-use.
        return std::forward<MemType>(mem);
    }

private:
    ZSTD_CCtx* context;
    std::unique_ptr<char[]> mem;
    ZSTD_outBuffer outBuf;

    void prepareOutBuf() {
        if (outBuf.pos == outBuf.size) {
            mem = static_cast<Consumer *>(this)->onBestow(std::move(mem));
            if (!mem) {
                mem = std::make_unique<char[]>(outBuf.size);
            }
            outBuf.dst = mem.get();
            assert(outBuf.size == ZSTD_CStreamOutSize());
            outBuf.pos = 0;
        }
    }

    void checkZstdRes(std::size_t code) {
        if (ZSTD_isError(code)) {
            std::cerr << "ZSTD ERROR: " << ZSTD_getErrorName(code) << std::endl;
        }
    }
};
