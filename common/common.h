#ifndef COMMON_H
#define COMMON_H

#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

namespace cc
{
    struct Exception : public std::exception
    {
        std::string _w;
        explicit Exception(std::string what_) : _w(std::move(what_)) {}

        const char* what () const noexcept override
        {
            return _w.c_str();
        }
    };

    struct Value
    {
        virtual ~Value() = default;
    };

    template<typename T_,
            typename... Args>
    std::string variadic_string(const char* format,
                                T_ arg1,
                                Args... args)
    {
        int size = std::snprintf(nullptr, 0, format, arg1, args...);
        if(size <= 0)
        {
            throw Exception( "Error during formatting: " + std::string(format));
        }
        size += 1;

        auto buf = std::make_unique<char[]>(size);
        std::snprintf(buf.get(), size, format, arg1, args...);

        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }

    std::vector<std::string> split_string(const std::string &str, char delimiter);
}

#endif //COMMON_H
