#pragma once

#include <exception>
#include <string>

class BaseException : public std::exception {
public:
    virtual const char *what() const noexcept {
        return msg.c_str();
    }

protected:
    template <typename... ArgTypes>
    BaseException(const std::string &msg)
        : msg(msg)
    {}

    std::string msg;
};
