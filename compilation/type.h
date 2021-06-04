#ifndef CC_TYPE_H
#define CC_TYPE_H

#include <cc.h>

namespace cc
{
    struct PointerType;

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

        static const Type* get(Context* ctx, const std::string &name);

        PointerType* get_pointer_to() const;
        ~Type() override;

    protected:
        PointerType* pointer_to;
        Context* ctx;
        primitive_t basic_type;
        explicit Type(Context* ctx, primitive_t basic_type) :
        basic_type(basic_type), ctx(ctx), pointer_to(nullptr) {}
    };

    struct PointerType : public Type
    {
        explicit PointerType(const Type* pointed_type) :
        Type(pointed_type->get_ctx(), PTR), pointed_type(pointed_type) {}
        const Type* get_pointed_type() const { return pointed_type; }
        std::string as_string() const override;

    private:
        const Type* pointed_type;
    };

    struct StructType : public Type
    {
        std::string name;
        explicit StructType(Context* ctx, StructDecl* ast);
        int get_size() const override { return size; };
        int get_offset(const std::string& field_name) const;

        std::string as_string() const override;
        ~StructType() override;

    private:
        int size;
        std::vector<std::pair<TypeDecl*, int>> fields;
    };

    template<Type::primitive_t T>
    struct PrimitiveType : public Type
    {
        explicit PrimitiveType(Context* ctx) : Type(ctx, T) {}
    };
}

#endif //CC_TYPE_H
