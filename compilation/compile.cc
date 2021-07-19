#include "compile.h"
#include "cc.h"
#include "module.h"
#include "instruction.h"
#include <fstream>
#include <grammar/grammar.h>
#include <iostream>
#include <iomanip>
#include <debug/print_debug.h>

namespace cc
{
    Context* cc_ctx = nullptr;

    static void read_file(std::ifstream &t,
                          std::string &str)
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

    void Compiler::read(std::string &file_content)
    {
        std::ifstream input_file(filename);
        if (!input_file.is_open())
        {
            throw Exception("Failed to open file: " + filename);
        }

        read_file(input_file, file_content);
        lines = split_string(file_content, '\n');
    }

    bool Compiler::parse()
    {
        std::string file;
        read(file);

        cc_init();
        CCBuffers* buf = cc_allocate_buffers();

        cc_ctx = ctx;

        ast = cc_parse(buf, file.c_str());

        cc_free_buffers(buf);
        cc_free();

        cc_ctx = ctx;

        return not put_errors() && ast;
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

    void put_warnings_or_errors(const std::vector<ASTException> &l,
                                const std::vector<std::string> &lines,
                                const std::string &filename,
                                const std::string &message)
    {
        for (const auto& e : l)
        {
            int digit_with = count_digit(static_cast<int>(e.self.line));
            for (int i = static_cast<int>(e.self.line) - ERROR_CONTEXT_LINE_N; i < e.self.line; i++)
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

            // +2 to account for two extra spaces between number and line string
            std::cerr << std::string(e.self.col + digit_with + 2, '-')
                      << "^\n"
                      << filename << ":" << e.self.line << ":" << e.self.col
                      << " " << message << ": " << e.what() << "\n\n";
        }
    }

    bool Compiler::put_errors() const
    {
        const std::vector<ASTException>& errors = ctx->get_errors();
        const std::vector<ASTException>& warnings = ctx->get_warnings();

        put_warnings_or_errors(errors, lines, filename, "Error");
        put_warnings_or_errors(warnings, lines, filename, "Warning");

        ctx->clear_warnings();

        return !errors.empty();
    }

    static void resolution_pass_cb(ASTValue* self, Context* ctx)
    {
        self->resolution_pass(ctx);
    }

    bool Compiler::resolve()
    {
        ctx->start_scope_build();
        for (ASTGlobal* iter = ast; iter; iter = iter->next)
        {
            iter->traverse(reinterpret_cast<ASTValue::TraverseCB>(resolution_pass_cb), ctx, nullptr);
        }
        ctx->end_scope_build();

        return not put_errors();
    }

    void Compiler::dump_ast() const
    {
        std::stringstream ss;
        p(ss, ast);
        std::cout << ss.str();
    }

    void Compiler::dump_ir() const
    {
        std::stringstream ss;
        p(ss, ctx->get_module()->scope());
        std::cout << ss.str();
    }

    Compiler::~Compiler()
    {
        delete ctx;
        delete ast;
    }

    bool Compiler::execute()
    {
        if (not parse()) return false;
        dump_ast();

        if (not resolve()) return false;
        if (not ir()) return false;

        dump_ir();
        return true;
    }

    bool Compiler::ir()
    {
        /* Build everything */
        IRBuilder IRB;

        for (ASTGlobal* iter = ast; iter; iter = iter->next)
        {
            IRB.set_insertion_point(nullptr);
            iter->add(ctx, IRB);
        }

        return not put_errors();
    }
}
