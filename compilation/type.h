#ifndef CC_TYPE_H
#define CC_TYPE_H

#include <cc.h>

namespace cc
{
    struct Type : public Value
    {
        enum primitive_t
        {
            VOID,
            CHAR,
            I8,        //!< Unsigned is set by a qualifier
            I16,
            I32,
            I64,
            F32,
            F64,
            PTR,
            ENUM,
            STRUCT,
            FUNCTION,
            P_N
        };

        enum qual_t
        {
            STATIC,
            CONST,
            UNSIGNED,
        };

        virtual std::string as_string() const;
        virtual int get_size() const;
        Context* get_ctx() const { return ctx; }

        static const Type* create(Context* ctx, const std::string &name);

    protected:
        Context* ctx;
        primitive_t basic_type;
        explicit Type(Context* ctx, primitive_t basic_type) : basic_type(basic_type), ctx(ctx) {}
    };

    struct StructType : public Type
    {
        explicit StructType(Context* ctx, FieldDecl* fields_ast);
        int get_size() const override { return size; };
        int get_offset(const std::string& field_name) const;

    private:
        int size;
        std::vector<std::tuple<std::string, const Type*, int>> fields;
    };

    template<Type::primitive_t T>
    struct PrimitiveType : public Type
    {
        explicit PrimitiveType(Context* ctx) : Type(ctx, T) {}
    };
}

#endif //CC_TYPE_H
