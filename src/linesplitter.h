#pragma once

#include <vector>

template <typename Consumer>
class LineSplitter : public Consumer {
public:
    LineSplitter(Consumer consumer)
        : Consumer(consumer)
    {}

    void consume(const char *data, std::size_t size) {
        std::size_t preSize = line.size();
        line.resize(preSize + size);
        for (std::size_t i = 0; i < size; i++) {
            if (data[i] == '\n') {
                static_cast<Consumer *>(this)->consume(line.data(), preSize + i);
                line.clear();
                consume(data + i + 1, size - i - 1);
                return;
            } else {
                line[preSize + i] = data[i];
            }
        }
    }

private:
    std::vector<char> line;
};
