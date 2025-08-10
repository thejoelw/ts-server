#pragma once

#include "baseexception.h"

class BadRequestException : public BaseException {
public:
    BadRequestException(const std::string &msg)
        : BaseException("Bad Request: " + msg)
    {}
};
