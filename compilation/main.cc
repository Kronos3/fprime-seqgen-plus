#include <iostream>
#include "compilation/compile.h"

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " [INPUT].c\n";
        return 1;
    }

    cc::Compiler compiler(argv[1]);
    if (not compiler.execute())
    {
        std::cerr << "Compiler execution failed\n";
        return 2;
    }

    return 0;
}
