#ifndef CC_COMPILE_H
#define CC_COMPILE_H

#include <cc.h>
#include "context.h"

namespace cc
{
    constexpr int ERROR_CONTEXT_LINE_N = 3;

    class IRBuilder
    {

    };

    class Compiler
    {
        GlobalDeclaration* ast;
        std::string filename;
        std::vector<std::string> lines;

        Context* ctx;

        void read(std::string& file_content);
        void handle_ast_exception(const ASTException& e,
                                  const std::string& phase) const;

        void resolve();

    public:
        explicit Compiler(std::string filename);
        void parse();
        void compile();

        void dump_ast();

        ~Compiler();
    };
}

#endif //CC_COMPILE_H
