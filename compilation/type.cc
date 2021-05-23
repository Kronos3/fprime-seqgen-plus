#include <map>
#include "type.h"
#include "context.h"

namespace cc
{
    int Type::get_size() const
    {
        switch (basic_type)
        {
            case VOID: return 0;
            case CHAR:
            case I8: return 1;
            case I16: return 2;
            case I32:
            case F32: return 4;
            case I64:
            case F64: return 8;
            case PTR: return ctx->get_ptr_size();
            case ENUM: return 4;
            default:
                throw Exception("Invalid type passed to Type::get_size()");
        }
    }

    const Type* Type::create(Context* ctx, const std::string &name)
    {
        if (name == "void") return ctx->type<VOID>();
        else if (name == "char") return ctx->type<CHAR>();
        else if (name == "i8") return ctx->type<I8>();
        else if (name == "i16") return ctx->type<I16>();
        else if (name == "i32") return ctx->type<I32>();
        else if (name == "i64") return ctx->type<I64>();
        else if (name == "f32") return ctx->type<F32>();
        else if (name == "f64") return ctx->type<F64>();

        // TODO Search for complex type decl
        return nullptr;
    }

    std::string Type::as_string() const
    {
        switch (basic_type)
        {
            case VOID: return "void";
            case CHAR: return "char";
            case I8: return "i8";
            case I16: return "i16";
            case I32: return "i32";
            case I64: return "i64";
            case F32: return "f32";
            case F64: return "f64";
            default:
                throw Exception("Invalid type passed to Type::get_size()");
        }
    }

    int StructType::get_offset(const std::string &field_name) const
    {
        for (const auto& iter : fields)
        {
            if (std::get<0>(iter) == field_name)
            {
                return std::get<2>(iter);
            }
        }

        throw Exception("Field not found in structure: " + field_name);
    }

    StructType::StructType(Context* ctx, FieldDecl* fields_ast) : Type(ctx, STRUCT)
    {
        size = 0;
        for (FieldDecl* iter = fields_ast; iter; iter = iter->next)
        {
            int current_size = fields_ast->type->get_size();

            // TODO We don't need to align to size
            //  (x86 doubles)
            if (size % current_size)
            {
                size += current_size - (size % current_size);
            }

            fields.emplace_back(std::move(fields_ast->name), fields_ast->type, size);
            size += current_size;
        }

        delete fields_ast;
    }
}
