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
        , bufSize(ZSTD_DStreamOutSize())
        , bufData(new char[bufSize])
    {
        checkZstdRes(ZSTD_CCtx_setParameter(context, ZSTD_c_compressionLevel, 7));
        checkZstdRes(ZSTD_CCtx_setParameter(context, ZSTD_c_checksumFlag, 1));
    }

    ~Compressor() {
        ZSTD_inBuffer input = { 0, 0, 0 };
        while (true) {
            ZSTD_outBuffer output = { bufData, bufSize, 0 };
            size_t const remaining = ZSTD_compressStream2(context, &output , &input, ZSTD_e_end);
            assert(output.dst == bufData);
            static_cast<Consumer *>(this)->consume(bufData, output.pos);

            if (!remaining) {
                break;
            }
        }

        delete[] bufData;
    }

    void consumeData(const char *data, std::size_t size) {
        ZSTD_inBuffer input = { data, size, 0 };
        do {
            ZSTD_outBuffer output = { bufData, bufSize, 0 };
            size_t const res = ZSTD_compressStream2(context, &output , &input, ZSTD_e_continue);
            checkZstdRes(res);
            assert(output.dst == bufData);
            static_cast<Consumer *>(this)->consume(bufData, output.pos);
        } while (input.pos != input.size);
    }

private:
    ZSTD_CCtx* context;
    std::size_t bufSize;
    char *bufData;

    void checkZstdRes(std::size_t code) {
        if (ZSTD_isError(code)) {
            std::cerr << "ZSTD ERROR: " << ZSTD_getErrorName(code) << std::endl;
        }
    }
};
