#pragma once

#include <vector>

#include "baseexception.h"

class DbParseException : public BaseException {
public:
    DbParseException(const std::string &msg)
        : BaseException(msg)
    {}

    static std::vector<DbParseException> &getStore() {
        static thread_local std::vector<DbParseException> store;
        return store;
    }
};
