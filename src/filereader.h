#pragma once

#include <fstream>

#include "baseexception.h"

template <typename Consumer>
class FileReader : public Consumer {
public:
    class OpenException : public BaseException {
    public:
        OpenException(const std::string &path)
            : BaseException("Could not open file for reading at " + path)
        {}
    };

    template <typename... ConsumerArgs>
    FileReader(ConsumerArgs... args)
        : Consumer(std::forward<ConsumerArgs>(args)...)
        , bufSize(static_cast<Consumer *>(this)->getPrefferedSize())
    {}

    template <typename ShouldRun>
    void read(const std::string &path, ShouldRun shouldRun) {
        std::ifstream hdl(path, std::ios::in | std::ios::binary);
        if (!hdl.is_open()) {
            throw OpenException(path);
        }
        do {
            if (!mem) {
                mem = std::make_unique<char[]>(bufSize);
            }
            hdl.read(mem.get(), bufSize);
            static_cast<Consumer *>(this)->onData(mem.get(), hdl.gcount());
            mem = static_cast<Consumer *>(this)->onBestow(std::move(mem));
        } while (!hdl.eof() && shouldRun());
        hdl.close();
    }

private:
    std::size_t bufSize;
    std::unique_ptr<char[]> mem;
};
