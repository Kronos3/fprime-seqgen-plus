#define NEOAST_GET_TOKENS
#define NEOAST_GET_STRUCTURE

#include <cstring>

#include "neoast_parser__cc_lib.h"

typedef int (* keyword_cb)(const char*, NeoastUnion* self);

using namespace cc;

static struct KeywordDecl_
{
    const char* name;
    int value;
} keywords[] = {
        {"if",       IF},
        {"else",     ELSE},
        {"for",      FOR},
        {"while",    WHILE},
        {"continue", CONTINUE},
        {"break",    BREAK},
        {"return",   RETURN},
        {nullptr}
};

int handle_keyword(const char* text)
{
    for (struct KeywordDecl_* iter = keywords; iter->name; iter++)
    {
        if (strcmp(iter->name, text) == 0)
        {
            return iter->value;
        }
    }

    return -1;
}
