#pragma once

#include <assert.h>
#include <cstdlib>
#include <iostream>

#include <zstd.h>

template <typename Consumer>
class Decompressor : public Consumer {
public:
    Decompressor(Consumer consumer)
        : Consumer(consumer)
        , context(ZSTD_createDCtx())
        , bufSize(ZSTD_DStreamOutSize())
        , bufData(new char[bufSize])
        , bufUsed(0)
    {}

    ~Decompressor() {
        delete[] bufData;
    }

    std::size_t getPrefferedSize() const {
        return ZSTD_DStreamInSize();
    }

    void consume(const char *data, std::size_t size) {
        ZSTD_inBuffer input = { data, size, 0 };
        while (input.pos < input.size) {
            ZSTD_outBuffer output = { bufData, bufSize, bufUsed };
            size_t const res = ZSTD_decompressStream(context, &output, &input);
            checkZstdRes(res);
            assert(output.dst == bufData);
            static_cast<Consumer *>(this)->consumeData(bufData + bufUsed, output.pos - bufUsed);
            if (output.pos == bufSize) {
                static_cast<Consumer *>(this)->consumeBestow(bufData);
                bufData = new char[bufSize];
                bufUsed = 0;
            } else {
                bufUsed = output.pos;
            }
        }
    }

private:
    ZSTD_DCtx* context;
    std::size_t bufSize;
    char *bufData;
    std::size_t bufUsed;

    void checkZstdRes(std::size_t code) {
        if (ZSTD_isError(code)) {
            std::cerr << "ZSTD ERROR: " << ZSTD_getErrorName(code) << std::endl;
        }
    }
};
