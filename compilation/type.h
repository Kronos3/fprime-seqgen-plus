#ifndef CC_TYPE_H
#define CC_TYPE_H
#include <common/common.h>

namespace cc
{
    class Context;
    struct PointerType;

    struct Type : public Value
    {
        enum primitive_t
        {
            VOID,
            I8,        //!< Unsigned is set by a qualifier
            I16,
            I32,
            I64,
            F32,
            F64,
            CHAR,
            PTR,
            ENUM,
            STRUCT,
            FUNCTION,
            P_N
        };

        virtual std::string as_string() const;
        virtual int get_size() const;
        Context* get_ctx() const { return ctx; }

        static const Type* get(Context* ctx, const std::string &name);

        PointerType* get_pointer_to() const;
        ~Type() override;

        virtual bool is_const() const { return false; }
        virtual bool is_unsigned() const { return false; }
        virtual bool is_volatile() const { return false; }

    protected:
        PointerType* pointer_to;
        Context* ctx;
        primitive_t basic_type;
        explicit Type(Context* ctx, primitive_t basic_type) :
        basic_type(basic_type), ctx(ctx), pointer_to(nullptr) {}
    };

    struct QualType : public Type
    {
        enum qual_t
        {
            NONE = 0,
            CONST  = 0x1,
            UNSIGNED = 0x2,
            VOLATILE = 0x4,
        };

        QualType(Context* ctx, int starting, const Type* type);
        static qual_t get(Context* ctx, const std::string &name);

        bool is_const() const override { return qualifiers & CONST; }
        bool is_unsigned() const override { return qualifiers & UNSIGNED; }
        bool is_volatile() const override { return qualifiers & VOLATILE; }

    private:
        int qualifiers;
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
