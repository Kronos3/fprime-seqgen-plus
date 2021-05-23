#ifndef CC_COMPILE_H
#define CC_COMPILE_H

#include <cc.h>
#include "context.h"

namespace cc
{
    constexpr int ERROR_CONTEXT_LINE_N = 3;

    class Compiler
    {
        ASTGlobal* ast;
        std::string filename;
        std::vector<std::string> lines;

        Context* ctx;

        void read(std::string& file_content);

        bool parse();
        bool resolve();
        bool ir();
        bool put_errors() const;

    public:
        explicit Compiler(std::string filename);

        bool execute();
        void dump_ast() const;

        ~Compiler();

        void dump_ir() const;
    };
}

#endif //CC_COMPILE_H
