#include "memory.h"

#include <new>
#include <map>
#include <iostream>

static std::map<std::size_t, std::size_t> &getSizeMap() {
    static std::map<std::size_t, std::size_t> map;
    return map;
}

static bool &isAllocating() {
    static thread_local bool flag = false;
    return flag;
}

static std::mutex &getMutex() {
    static std::mutex mutex;
    return mutex;
}

void *operator new(std::size_t size) noexcept(false) {
    assert(size > 0);
    std::size_t *res = static_cast<std::size_t *>(std::malloc(sizeof(std::size_t) * 2 + size));
    if (isAllocating()) {
        res[0] = 0;
    } else {
        std::lock_guard<std::mutex> lock(getMutex());
        isAllocating() = true;
        getSizeMap()[size]++;
        isAllocating() = false;

        res[0] = size;
    }
    return res + 2;
}

void operator delete(void *ptr) throw() {
    assert(!isAllocating());
    std::size_t *res = static_cast<std::size_t *>(ptr) - 2;
    std::size_t size = res[0];
    if (size > 0) {
        std::lock_guard<std::mutex> lock(getMutex());
        getSizeMap()[size]--;
    }
    std::free(res);
}

void *operator new[](std::size_t size) noexcept(false) {
    return ::operator new(size);
}

void operator delete[](void *ptr) throw() {
    ::operator delete(ptr);
}

void printSizeDist() {
    for (std::pair<std::size_t, std::size_t> pair : getSizeMap()) {
        std::cout << pair.first << ": " << pair.second << ", ";
    }
    std::cout << std::endl << std::endl;
}
