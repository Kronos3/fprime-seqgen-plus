#include <map>
#include <cc.h>

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
            case F64:
            case PTR: return 8;  /* AHHHHHHHH */
            case ENUM: return 4;
            default:
                throw Exception("Invalid type passed to Type::get_size()");
        }
    }

    const Type* Type::get(Context* ctx, const std::string &name)
    {
        // Builtin types
        if (name == "void") return ctx->type<VOID>();
        else if (name == "char") return ctx->type<CHAR>();
        else if (name == "i8") return ctx->type<I8>();
        else if (name == "u8") return ctx->unsigned_type<I8>();
        else if (name == "i16") return ctx->type<I16>();
        else if (name == "u16") return ctx->unsigned_type<I16>();
        else if (name == "i32") return ctx->type<I32>();
        else if (name == "u32") return ctx->unsigned_type<I32>();
        else if (name == "i64") return ctx->type<I64>();
        else if (name == "u64") return ctx->unsigned_type<I64>();
        else if (name == "f32") return ctx->type<F32>();
        else if (name == "f64") return ctx->type<F64>();

        return ctx->type(name);
    }

    QualType::qual_t QualType::get(Context* ctx, const std::string &name)
    {
        (void) ctx;

        if (name == "const") return QualType::qual_t::CONST;
        else if (name == "volatile") return QualType::qual_t::VOLATILE;
        else if (name == "unsigned") return QualType::qual_t::UNSIGNED;

        return NONE;
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

    std::string PointerType::as_string() const
    {
        return pointed_type->as_string() + "*";
    }

    PointerType* Type::get_pointer_to() const
    {
        Type* r = const_cast<Type*>(this);

        if (!pointer_to)
        {
            auto * out = new PointerType(this);
            r->pointer_to = out;
            return out;
        }

        return pointer_to;
    }

    Type::~Type()
    {
        delete pointer_to;
    }

    int StructType::get_offset(const std::string &field_name) const
    {
        for (const auto& iter : fields)
        {
            if (iter.first->name == field_name)
            {
                return iter.second;
            }
        }

        throw Exception("Field not found in structure: " + field_name);
    }

    StructType::StructType(Context* ctx, StructDecl* ast) :
    Type(ctx, STRUCT), name(ast->name)
    {
        size = 0;
        for (FieldDecl* iter = ast->fields; iter; iter = iter->next)
        {
            int current_size = iter->decl->type->get_size();

            // TODO We don't need to align to size
            //  (x86 doubles)
            if (size % current_size)
            {
                size += current_size - (size % current_size);
            }

            fields.emplace_back(iter->decl, size);
            size += current_size;
        }
    }

    StructType::~StructType()
    {
        for (auto& iter : fields)
        {
            delete iter.first;
        }
        fields.clear();
    }

    std::string StructType::as_string() const
    {
        return name;
    }

    StructDecl::StructDecl(const ASTPosition* position,
                           Context* ctx, std::string name, FieldDecl* fields) :
            ASTGlobal(position), name(std::move(name)), fields(fields), type(ctx->declare_structure(this)) {}

    TypeDecl::TypeDecl(
            Context* ctx,
            const ASTPosition* position,
            const char* type_ident, const char* name_) :
            ASTValue(position), type(nullptr), variable(nullptr)
    {
        TAKE_STRING(name, name_);
        ctx->emit_error(position, "Unresolved type '" + std::string(type_ident) + "'");
        free(const_cast<char*>(type_ident));
    }

    QualType::QualType(Context* ctx, int starting, const Type* type)
            : Type(*type), qualifiers(starting)
    {
        ctx->register_type(this);
    }

    const Type* NumericExpr::get_type(Context* ctx) const
    {
        switch(type)
        {
            case ASCII:
                return ctx->type<Type::CHAR>();
            case INTEGER:
            {
                if (value.integer > INT32_MAX || value.integer < INT32_MIN)
                {
                    return ctx->type<Type::I64>();
                }
                else
                {
                    return ctx->type<Type::I32>();
                }
            }
            case FLOATING:
                // Floating point literals are always doubles in C
                return ctx->type<Type::F64>();
        }

        throw Exception("Invalid NumericExpr");
    }

    const Type* LiteralExpr::get_type(Context* ctx) const
    {
        return nullptr;
    }
}
