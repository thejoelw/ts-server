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

    void consume(const char *data, std::size_t size) {
        hdl.write(data, size);
    }

    ~FileWriter() {
        hdl.close();
    }

private:
    std::ofstream hdl;
};
