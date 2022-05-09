#include "compile.h"
#include "cc.h"
#include "module.h"
#include "instruction.h"
#include <fstream>
#include <grammar/grammar.h>
#include <iostream>
#include <debug/print_debug.h>

namespace cc
{
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

        ast = cc_parse(ctx, buf, file.c_str());

        cc_free_buffers(buf);
        cc_free();

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

    static void put_position(const ASTPosition& position,
                             const std::vector<std::string>& lines,
                             std::ostream& os)
    {
        if (position.line < 0)
        {
            return;
        }

        // Get the number of digits in the line number:
        int dig_n = snprintf(nullptr, 0, "%+d", position.line);

        for (int i = (int) position.line - ERROR_CONTEXT_LINE_N; i < (int) position.line; i++)
        {
            if (i < 0)
            {
                continue;
            }

            os << variadic_string("% *d | %s\n", dig_n, i + 1,
                                  lines[i].c_str());
        }

        if (position.line > 0)
        {
            os << std::string(position.col + 3 + dig_n, ' ')
               << "\033[1;32m^"
               << std::string(position.len - 1, '~')
               << "\033[0m\n";
        }
    }

    static void put_warnings_or_errors(const std::vector<ASTException> &l,
                                       const std::vector<std::string> &lines,
                                       const std::string &filename,
                                       const std::string &message,
                                       std::ostream& os)
    {
        for (const auto& e : l)
        {
            os << "\033[1m" << filename << ":" << e.self.line << ":" << e.self.col + 1
                      << " " << message << ": " << e.what() << "\n";
            put_position(e.self, lines, os);
            os << "\n";
        }
    }

    bool Compiler::put_errors() const
    {
        const std::vector<ASTException>& errors = ctx->get_errors();
        const std::vector<ASTException>& warnings = ctx->get_warnings();

        put_warnings_or_errors(errors, lines, filename, "\033[1;31merror:\033[1;0m ", std::cout);
        put_warnings_or_errors(warnings, lines, filename, "\033[1;33mwarning:\033[1;0m ", std::cout);

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
