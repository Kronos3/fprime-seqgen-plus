#include "compile.h"
#include "cc.h"
#include <fstream>
#include <grammar/grammar.h>
#include <iostream>
#include <iomanip>
#include <debug/print_debug.h>

namespace cc
{
    static void read_file(std::ifstream& t,
                          std::string& str)
    {
        t.seekg(0, std::ios::end);
        str.reserve(t.tellg());
        t.seekg(0, std::ios::beg);

        str.assign((std::istreambuf_iterator<char>(t)),
                   std::istreambuf_iterator<char>());
    }

    Compiler::Compiler(std::string filename) :
        ast(nullptr), filename(std::move(filename)), ctx(new Context())
    {
    }

    void Compiler::parse()
    {
        std::string file;
        read(file);

        cc_init();
        CCBuffers* buf = cc_allocate_buffers();

        try
        {
            ast = cc_parse(buf, file.c_str());
        }
        catch (cc::ASTException& e)
        {
            handle_ast_exception(e, "parsing");
        }
        catch (cc::Exception& e)
        {
            std::cerr << "Exception occurred during parsing: "
                      << e.what();
        }

        cc_free_buffers(buf);
        cc_free();
    }

    static int count_digit(int n)
    {
        int count = 0;
        while (n != 0)
        {
            n = n / 10;
            ++count;
        }
        return count;
    }

    void Compiler::read(std::string& file_content)
    {
        std::ifstream input_file(filename);
        if (!input_file.is_open())
        {
            throw Exception("Failed to open file: " + filename);
        }

        read_file(input_file, file_content);
        lines = split_string(file_content, '\n');
    }

    void Compiler::handle_ast_exception(const ASTException &e, const std::string& phase) const
    {
        std::cerr << "Exception occurred during " << phase << ": "
                  << e.what()
                  << "\n";

        int digit_with = count_digit(static_cast<int>(e.self->line));
        for (int i = static_cast<int>(e.self->line) - ERROR_CONTEXT_LINE_N; i < e.self->line; i++)
        {
            if (i < 0)
            {
                continue;
            }

            std::cerr
                    << std::setfill('0') << std::setw(digit_with)
                    << i + 1
                    << "  "
                    << lines[i]
                    << "\n";
        }

        std::cerr << std::string(e.self->start_col + digit_with + 1, '-')
                  << "^\n\n";
    }

    static void resolution_pass_cb(ASTValue* self, Context* ctx)
    {
        self->resolution_pass(ctx);
    }

    void Compiler::compile()
    {
        try
        {
            resolve();
        }
        catch (cc::ASTException& e)
        {
            handle_ast_exception(e, "resolution");
        }
        catch (cc::Exception& e)
        {
            std::cerr << "Exception occurred during resolution: "
                      << e.what();
        }
    }

    void Compiler::resolve()
    {
        ctx->start_scope_build();
        ast->traverse(reinterpret_cast<ASTValue::TraverseCB>(resolution_pass_cb), ctx, nullptr);
        ctx->end_scope_build();
    }

    void Compiler::dump_ast()
    {
        std::stringstream ss;
        p(ss, ast);
        std::cout << ss.str();
    }

    Compiler::~Compiler()
    {
        delete ctx;
        delete ast;
    }


}
