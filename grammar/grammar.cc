#define NEOAST_EXTERNAL_INCLUDE
#include "neoast_parser__cc_lib.cc"

typedef int (*keyword_cb)(const char*, NeoastUnion* self);

using namespace cc;

static struct TypeDecl_
{
    const char* name;
    type_t value;
} types[] = {
        {"F32", TYPE_F64},
        {"F64", TYPE_F64},
        {"I32", TYPE_I32},
        {"I64", TYPE_I32},
        {"void", TYPE_VOID},
        {nullptr}
};

static int handle_type(const char* name, NeoastUnion* self)
{
    for (struct TypeDecl_* iter = types; iter->name; iter++)
    {
        if (strcmp(iter->name, name) == 0)
        {
            self->type = iter->value;
            return TYPE;
        }
    }

    return -1;
}

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
        {nullptr}
};

int handle_keyword(const char* text, void* yyval)
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

    // Is it a type?
    return handle_type(text, static_cast<NeoastUnion*>(yyval));
}
