#ifndef CC_TYPE_H
#define CC_TYPE_H
#include <common/common.h>

namespace cc
{
    class Context;
    struct PointerType;
    struct StructDecl;
    struct TypeDecl;

    struct Type : public Value
    {
        enum primitive_t
        {
            I8,
            U8,
            I16,
            U16,
            I32,
            U32,
            I64,
            U64,
            F32,
            F64,
            CHAR,
            P_N
        };

        virtual std::string as_string() const;
        virtual int get_size() const;
        Context* get_ctx() const { return ctx; }

        static const Type* get(Context* ctx, const std::string &name);

        ~Type() override;

    protected:
        PointerType* pointer_to;
        Context* ctx;
        primitive_t basic_type;
        explicit Type(Context* ctx, primitive_t basic_type) :
        basic_type(basic_type), ctx(ctx), pointer_to(nullptr) {}
    };
}

#endif //CC_TYPE_H
