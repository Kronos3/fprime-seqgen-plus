#ifndef COMMON_H
#define COMMON_H

#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

namespace cc
{

#define ALL_OPERATORS_VIRTUAL \
    virtual Constant* operator+(const Constant* c) const = 0; \
    virtual Constant* operator-(const Constant* c) const = 0; \
    virtual Constant* operator/(const Constant* c) const = 0; \
    virtual Constant* operator*(const Constant* c) const = 0; \
    virtual Constant* operator&(const Constant* c) const = 0; \
    virtual Constant* operator|(const Constant* c) const = 0; \
    virtual Constant* operator^(const Constant* c) const = 0; \
    virtual Constant* operator<(const Constant* c) const = 0; \
    virtual Constant* operator>(const Constant* c) const = 0; \
    virtual Constant* operator<=(const Constant* c) const = 0; \
    virtual Constant* operator>=(const Constant* c) const = 0; \
    virtual Constant* operator==(const Constant* c) const = 0; \
    virtual Constant* operator!=(const Constant* c) const = 0; \
    virtual Constant* operator&&(const Constant* c) const = 0; \
    virtual Constant* operator||(const Constant* c) const = 0; \
    virtual Constant* operator~() const = 0; \
    virtual Constant* operator!() const = 0;

#define ALL_OPERATORS_DECL \
    Constant* operator+(const Constant* c) const override; \
    Constant* operator-(const Constant* c) const override; \
    Constant* operator/(const Constant* c) const override; \
    Constant* operator*(const Constant* c) const override; \
    Constant* operator&(const Constant* c) const override; \
    Constant* operator|(const Constant* c) const override; \
    Constant* operator^(const Constant* c) const override; \
    Constant* operator<(const Constant* c) const override; \
    Constant* operator>(const Constant* c) const override; \
    Constant* operator<=(const Constant* c) const override; \
    Constant* operator>=(const Constant* c) const override; \
    Constant* operator==(const Constant* c) const override; \
    Constant* operator!=(const Constant* c) const override; \
    Constant* operator&&(const Constant* c) const override; \
    Constant* operator||(const Constant* c) const override; \
    Constant* operator~() const override; \
    Constant* operator!() const override;

    struct Exception : public std::exception
    {
        std::string _w;
        explicit Exception(std::string what_) : _w(std::move(what_)) {}

        const char* what () const noexcept override
        {
            return _w.c_str();
        }
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

    struct Value
    {
        virtual ~Value() = default;
    };

    class IR : public Value
    {
        int value_id;
    public:
        IR()
        {
            static int value_id_c = 0;
            value_id = value_id_c++;
        }

        int get_id() const { return value_id; }
        virtual std::string as_string() const { return variadic_string("%%%d", get_id()); }
    };

    struct Constant : public IR
    {
        virtual size_t get_size() const = 0;
        virtual void write(void* buffer) const = 0;
        virtual Constant* copy() const = 0;

        ALL_OPERATORS_VIRTUAL
    };

    struct Reference : public IR
    {
    };
}

#endif //COMMON_H
