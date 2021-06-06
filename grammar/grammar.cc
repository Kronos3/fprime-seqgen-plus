#define NEOAST_EXTERNAL_INCLUDE
#include "neoast_parser__cc_lib.cc"
#include "compilation/type.h"

typedef int (*keyword_cb)(const char*, NeoastUnion* self);

using namespace cc;

static struct KeywordDecl_ {
    const char* name;
    int value;
    keyword_cb cb;
} keywords[] = {
        {"if", IF},
        {"else", ELSE},
        {"for", FOR},
        {"while", WHILE},
        {"continue", CONTINUE},
        {"break", BREAK},
        {"return", RETURN},
        {"struct", STRUCT},
        {nullptr}
};

int handle_keyword(Context* ctx, const char* text, void* yyval)
{
    for (struct KeywordDecl_* iter = keywords; iter->name; iter++)
    {
        if (strcmp(iter->name, text) == 0)
        {
            if (iter->cb)
            {
                return iter->cb(text, static_cast<NeoastUnion*>(yyval));
            }

            return iter->value;
        }
    }

    const Type* type = Type::get(ctx, text);
    if (type)
    {
        static_cast<NeoastUnion*>(yyval)->type = type;
        return TYPENAME;
    }

    QualType::qual_t q = QualType::get(ctx, text);
    if (q != cc::QualType::NONE)
    {
        static_cast<NeoastUnion*>(yyval)->qualifier_prim = q;
        return QUALIFIER;
    }

    return -1;
}
