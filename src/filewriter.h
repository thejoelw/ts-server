#pragma once

#include <fstream>

class FileWriter {
public:
    class OpenException : public std::exception {};

    FileWriter(const std::string &path) {
        hdl.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!hdl.is_open()) {
            throw OpenException();
        }
    }

    void onData(const char *data, std::size_t size) {
        assert(size > 0);
        hdl.write(data, size);
    }

    template <typename MemType>
    MemType onBestow(MemType mem) {
        // Don't need the memory any more (already consumed by write), so can immediately give back to producer for re-use.
        return std::forward<MemType>(mem);
    }

    ~FileWriter() {
        hdl.close();
    }

private:
    std::ofstream hdl;
};
