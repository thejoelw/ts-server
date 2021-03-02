#pragma once

#include <fstream>

template <typename Consumer>
class FileReader : public Consumer {
public:
    class OpenException : public std::exception {};

    template <typename... ConsumerArgs>
    FileReader(ConsumerArgs... args)
        : Consumer(std::forward<ConsumerArgs>(args)...)
        , bufSize(static_cast<Consumer *>(this)->getPrefferedSize())
        , bufData(new char[bufSize])
    {}

    ~FileReader() {
        delete[] bufData;
    }

    void read(const std::string &path) {
        std::ifstream hdl(path, std::ios::in | std::ios::binary);
        if (!hdl.is_open()) {
            throw OpenException();
        }
        do {
            hdl.read(bufData, bufSize);
            static_cast<Consumer *>(this)->consume(bufData, hdl.gcount());
        } while (!hdl.eof());
        hdl.close();
    }

private:
    std::size_t bufSize;
    char *bufData;
};
