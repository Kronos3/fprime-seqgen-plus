#include <map>

#include "type.h"
#include "context.h"

namespace cc
{
    int Type::get_size() const
    {
        switch (basic_type)
        {
            case CHAR:
            case U8:
            case I8: return 1;
            case U16:
            case I16: return 2;
            case U32:
            case I32:
            case F32: return 4;
            case U64:
            case I64:
            case F64:
            default:
                throw Exception("Invalid type passed to Type::get_size()");
        }
    }

    const Type* Type::get(Context* ctx, const std::string &name)
    {
        // Builtin types
        if (name == "char") return ctx->type<CHAR>();
        else if (name == "I8") return ctx->type<I8>();
        else if (name == "U8") return ctx->type<U8>();
        else if (name == "I16") return ctx->type<I16>();
        else if (name == "U16") return ctx->type<U16>();
        else if (name == "I32") return ctx->type<I32>();
        else if (name == "U32") return ctx->type<U32>();
        else if (name == "I64") return ctx->type<I64>();
        else if (name == "U64") return ctx->type<U64>();
        else if (name == "F32") return ctx->type<F32>();
        else if (name == "F64") return ctx->type<F64>();

        return ctx->type(name);
    }
}
