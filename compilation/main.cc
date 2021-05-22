#include <iostream>
#include "compilation/compile.h"

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s [INPUT].c\n", argv[0]);
        return 1;
    }

    cc::Compiler compiler(argv[1]);
    compiler.parse();
    compiler.compile();

    compiler.dump_ast();

    return 0;
}
