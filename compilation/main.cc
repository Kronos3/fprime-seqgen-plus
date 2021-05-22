#include <iostream>
#include "cc.h"
#include "debug/print_debug.h"
#include "grammar/grammar.h"
#include <fstream>
#include <streambuf>
#include "compilation/context.h"

void read_file(std::ifstream& t,
               std::string& str)
{
    t.seekg(0, std::ios::end);
    str.reserve(t.tellg());
    t.seekg(0, std::ios::beg);

    str.assign((std::istreambuf_iterator<char>(t)),
               std::istreambuf_iterator<char>());
}

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s [INPUT].c\n", argv[0]);
        return 1;
    }

    cc_init();

    std::ifstream input_file(argv[1]);
    std::string file;
    read_file(input_file, file);

    CCBuffers* buf = cc_allocate_buffers();
    cc::GlobalDeclaration* parsed = cc_parse(buf, file.c_str());

    if (parsed)
    {
        std::stringstream ss;
        p(ss, parsed);
        std::cout << ss.str();

        auto* ctx = new Context();
        perform_resolution(parsed, ctx);
        delete ctx;
    }

    cc_free_buffers(buf);
    cc_free();

    if (!parsed)
    {
        fprintf(stderr, "Failed to parse file");
        return 1;
    }

    delete parsed;
    return 0;
}
