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

    // Try to parse the typename
    const Type* parsed = Type::create(ctx, text);
    if (parsed)
    {
        static_cast<NeoastUnion*>(yyval)->type = parsed;
        return TYPE;
    }

    return -1;
}
